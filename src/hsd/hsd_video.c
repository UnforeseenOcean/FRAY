#include "hsd_video.h"

static u32 HSD_VINumXFB = 0; 
HSD_VIInfo HSD_VIData;
HSD_VIStatus vi;

#define _p	((HSD_VIInfo *)&HSD_VIData)

static u8 garbage[HSD_ANTIALIAS_GARBAGE_SIZE];

//8037588C
static s32 HSD_VISearchXFBByStatus(HSD_VIXFBDrawDispStatus status){
	for (u32 i = 0; i < HSD_VI_XFB_MAX; i++){
		if (_p->xfb[i].status == status) 
		return i;
	}
	return -1;
}

//803758DC
VIRetraceCallback HSD_VISetUserPreRetraceCallback(VIRetraceCallback cb){
	u32 intr;
	
	VIRetraceCallback old = _p->pre_cb;
	intr = IRQ_Disable();
	_p->pre_cb = cb;
	IRQ_Restore(intr);
	
	return old;
}

//80375934
VIRetraceCallback HSD_VISetUserGXDrawDoneCallback(VIRetraceCallback cb){
	u32 intr;
	
	VIRetraceCallback old = _p->post_cb;
	intr = IRQ_Disable();
	_p->post_cb = cb;
	IRQ_Restore(intr);
	
	return old;
}

//8037598C
HSD_VIGXDrawDoneCallback HSD_VISetUserGXDrawDoneCallback(HSD_VIGXDrawDoneCallback cb){
	u32 intr;
	
	HSD_VIGXDrawDoneCallback old = _p->drawdone.cb;
	intr = IRQ_Disable();
	_p->drawdone.cb = cb;
	IRQ_Restore(intr);
	
	return old;
}

//80375E1C
static void HSD_VIGXDrawDoneCB(){
	_p->drawdone.waiting = 0;
	
	if (_p->drawdone.cb)
		_p->drawdone.cb(_p->drawdone.arg);
}

//80375E60
static BOOL HSD_VIGetDrawDoneWaitingFlag(){
  return _p->drawdone.waiting;
}

//80375F74
void HSD_VICopyEFB2XFBPtr(HSD_VIStatus *vi, void *buffer, HSD_RenderPass rpass)
{
  GXRModeObj *rmode = &vi->rmode;
  u32 n_xfb_lines;
  u16 lines;
  u32 offset;

  GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, vi->vf, rmode->vfilter);
  GX_SetDispCopyGamma(vi->gamma);

  HSD_StateSetColorUpdate(vi->update_clr);
  HSD_StateSetAlphaUpdate(vi->update_alpha);
  HSD_StateSetZMode(vi->update_z, GX_LEQUAL, GX_TRUE);

  GX_SetCopyClear(vi->clear_clr, vi->clear_z);

  switch (rpass) {
  case HSD_RP_SCREEN:
    GX_SetCopyClamp((GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    n_xfb_lines = GX_SetDispCopyYScale((f32)(rmode->xfbHeight)
				      / (f32)(rmode->efbHeight)); 
    GX_SetDispCopyDst(rmode->fbWidth, n_xfb_lines);
    GX_CopyDisp(buffer, GX_TRUE);
    break;

  case HSD_RP_TOPHALF:
	GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight - HSD_ANTIALIAS_OVERLAP);
	n_xfb_lines = GX_SetDispCopyYScale(1.0);
	GX_SetDispCopyDst(rmode->fbWidth, n_xfb_lines);
    GX_SetCopyClamp(GX_CLAMP_TOP);
    lines = rmode->efbHeight - HSD_ANTIALIAS_OVERLAP;
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, lines);
    GX_CopyDisp(buffer, GX_TRUE);
    return;

  case HSD_RP_BOTTOMHALF:
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight - HSD_ANTIALIAS_OVERLAP);
	GX_SetDispCopyDst(rmode->fbWidth, GX_SetDispCopyYScale(1.0));
    GX_SetCopyClamp(GX_CLAMP_BOTTOM);
	lines = rmode->efbHeight - HSD_ANTIALIAS_OVERLAP;
	GX_SetDispCopySrc(0, HSD_ANTIALIAS_OVERLAP, rmode->fbWidth, lines);
    offset = (VIDEO_PadFramebufferWidth(rmode->fbWidth) * lines * (u32) VI_DISPLAY_PIX_SZ);
    GX_CopyDisp((void *)((u32)buffer + offset), GX_TRUE);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, HSD_ANTIALIAS_OVERLAP);
    GX_SetCopyClamp((GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
    GX_CopyDisp((void *)garbage, GX_TRUE);
    break;

  default:
    //HSD_PANIC("unexpected type of render pass.\n")
  }

  GX_PixModeSync();
}

//803761C0
void HSD_VICopyXFBASync(HSD_RenderPass rpass){
	s32 idx = -1;
	
	if (HSD_VIGetNumXFB() >= 2){
		while((idx = HSD_VIGetXFBDrawEnable()) == -1)
			VIDEO_WaitVSync();

		HSD_VICopyEFB2XFBPtr(&vi, HSD_VIGetXFBPtr(idx), rpass);
		
		u32 intr;
  		intr = IRQ_Disable();
		assert(_p->xfb[idx].status == HSD_VI_XFB_DRAWING);
		_p->xfb[idx].status = HSD_VI_XFB_WAITDONE;
		_p->xfb[idx].vi_all = _p->current;
		_p->current.chg_flag = 0;
		IRQ_Restore(intr);
	
		while (HSD_VIGetDrawDoneWaitingFlag())
			GX_WaitDrawDone();
		_p->drawdone.waiting = 1;
		_p->drawdone.arg = idx;
		GXSetDrawDone();
	}
}

//803762C4
void HSD_VIDrawDoneXFB(s32 idx){
	u32 intr = IRQ_Disable();
	
	assert(_p->xfb[idx].status == HSD_VI_XFB_WAITDONE);
	_p->xfb[idx].status = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT) != -1 ? HSD_VI_XFB_DRAWDONE : HSD_VI_XFB_NEXT;
	
	IRQ_Restore(intr);
}

//8037639C
void HSD_VISetXFBDrawDone(){
	u32 intr;
	s32 idx;
	if (HSD_VIGetNumXFB() >= 2){
		intr = IRQ_Disable();
		
		idx  = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWING);
		assert(idx != -1);
		HSD_VISetXFBWaitDone(idx);
		HSD_VIDrawDoneXFB(idx);
		
		IRQ_Restore(intr);
	}
}

//807364A8
s32 HSD_VIGetXFBLastDrawDone()
{
	s32 idx = -1;
	u32 intr = IRQ_Disable();
	if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_WAITDONE)) == -1)
		if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE)) == -1)
			if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT)) == -1)
				idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY);
	IRQ_Restore(intr);

  return idx;
}

//80375E70
s32 HSD_VIGetXFBDrawEnable(){
	u32 intr;
	s32 idx = -1;
	
	if (HSD_VIGetNumXFB() >= 2){
		intr = IRQ_Disable();
		if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWING)) == -1)
			if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE)) != -1)
				_p->xfb[idx].status = HSD_VI_XFB_DRAWING;
	
		IRQ_Restore(intr);
	}
	return idx;
}

//80376718
void HSD_VISetConfigure(GXRModeObj *rmode)
{
  _p->current.vi.rmode = *rmode;
  _p->current.chg_flag = 1;
}

static u32 HSD_VIGetNumXFB(){
	return HSD_VINumXFB;
}