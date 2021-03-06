#include "hsd_video.h"

#include "hsd_init.h"
#include "hsd_state.h"

static u32 HSD_VINumXFB = 0;
HSD_VIInfo HSD_VIData;
HSD_VIStatus vi;

#define _p ((HSD_VIInfo*)&HSD_VIData)

static u8 garbage[HSD_ANTIALIAS_GARBAGE_SIZE] ATTRIBUTE_ALIGN(32);

static u32 HSD_VIGetNbXFB(void)
{
    return _p->nb_xfb;
}

//8037588C
static s32 HSD_VISearchXFBByStatus(HSD_VIXFBDrawDispStatus status)
{
    for (u32 i = 0; i < HSD_VI_XFB_MAX; i++) {
        if (_p->xfb[i].status == status)
            return i;
    }
    return -1;
}

//803758DC
VIRetraceCallback HSD_VISetUserPreRetraceCallback(VIRetraceCallback cb)
{
    u32 intr;

    VIRetraceCallback old = _p->pre_cb;
    intr = IRQ_Disable();
    _p->pre_cb = cb;
    IRQ_Restore(intr);

    return old;
}

//80375934
VIRetraceCallback HSD_VISetUserPostRetraceCallback(VIRetraceCallback cb)
{
    u32 intr;

    VIRetraceCallback old = _p->post_cb;
    intr = IRQ_Disable();
    _p->post_cb = cb;
    IRQ_Restore(intr);

    return old;
}

//8037598C
HSD_VIGXDrawDoneCallback HSD_VISetUserGXDrawDoneCallback(HSD_VIGXDrawDoneCallback cb)
{
    u32 intr;

    HSD_VIGXDrawDoneCallback old = _p->drawdone.cb;
    intr = IRQ_Disable();
    _p->drawdone.cb = cb;
    IRQ_Restore(intr);

    return old;
}

//803759E4
static void HSD_VIPreRetraceCB(u32 retraceCount)
{
    s32 idx;
    int flush = 0;
    int renew = 0;

    idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT);
    if (idx != -1) {
        VIDEO_SetNextFramebuffer(_p->xfb[idx].buffer);

        if (_p->xfb[idx].vi_all.chg_flag) {
            VIDEO_Configure(&_p->xfb[idx].vi_all.vi.rmode);
            VIDEO_SetBlack(_p->xfb[idx].vi_all.vi.black);
        }
        flush = 1;
        renew = 1;
    } else if (HSD_VIGetNbXFB() == 1 && _p->efb.status == HSD_VI_EFB_DRAWDONE) {

        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY)) == -1) {
            idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE);
            assert(idx != -1);
            VIDEO_SetNextFramebuffer(_p->xfb[idx].buffer);
            flush = 1;
        }
        _p->xfb[idx].status = HSD_VI_XFB_COPYEFB;

        if (_p->efb.vi_all.chg_flag) {
            VIDEO_Configure(&_p->efb.vi_all.vi.rmode);
            VIDEO_SetBlack(_p->efb.vi_all.vi.black);
            flush = 1;
        }
        renew = 1;
    }
    if (flush)
        VIDEO_Flush();

    {
        static u32 vr_count = 0;
        static u32 renew_count = 0;

        if (renew)
            renew_count++;
        if (++vr_count >= _p->perf.frame_period) {
            _p->perf.frame_renew = renew_count;
            vr_count = renew_count = 0;
        }
    }

    if (_p->pre_cb)
        _p->pre_cb(retraceCount);
}

//80375C34
static void HSD_VIPostRetraceCB(u32 retraceCount)
{
    s32 idx;
    s32 next;

    if ((next = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT)) != -1) {
        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY)) != -1) {
            _p->xfb[idx].status = HSD_VI_XFB_FREE;
        }
        _p->xfb[next].status = HSD_VI_XFB_DISPLAY;

        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE)) != -1) {
            _p->xfb[idx].status = HSD_VI_XFB_NEXT;
        }
    } else if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_COPYEFB)) != -1) {
        HSD_VICopyEFB2XFBPtr(&_p->efb.vi_all.vi, _p->xfb[idx].buffer, HSD_RP_SCREEN);
        _p->xfb[idx].status = HSD_VI_XFB_DISPLAY;
        _p->efb.status = HSD_VI_EFB_FREE;
    }

    if (_p->post_cb)
        _p->post_cb(retraceCount);
}

//80375E1C
static void HSD_VIGXDrawDoneCB(void)
{
    _p->drawdone.waiting = 0;

    if (_p->drawdone.cb)
        _p->drawdone.cb(_p->drawdone.arg);
}

//80375E60
__attribute__((noinline)) static s32 HSD_VIGetDrawDoneWaitingFlag(void)
{
    return _p->drawdone.waiting;
}

//80375E70
s32 HSD_VIGetXFBDrawEnable(void)
{
    u32 intr;
    s32 idx = -1;

    if (HSD_VIGetNbXFB() >= 2) {
        intr = IRQ_Disable();
        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWING)) == -1)
            if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE)) != -1)
                _p->xfb[idx].status = HSD_VI_XFB_DRAWING;

        IRQ_Restore(intr);
    }
    return idx;
}

//80375F74
void HSD_VICopyEFB2XFBPtr(HSD_VIStatus* vi, void* buffer, HSD_RenderPass rpass)
{
    GXRModeObj* rmode = &vi->rmode;
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
        n_xfb_lines = GX_SetDispCopyYScale((f32)(rmode->xfbHeight) / (f32)(rmode->efbHeight));
        GX_SetDispCopyDst(rmode->fbWidth, n_xfb_lines);
        GX_CopyDisp(buffer, GX_TRUE);
        break;

    case HSD_RP_TOPHALF:
        lines = rmode->efbHeight - HSD_ANTIALIAS_OVERLAP;
        GX_SetDispCopySrc(0, 0, rmode->fbWidth, lines);
        n_xfb_lines = GX_SetDispCopyYScale(1.0f);
        GX_SetDispCopyDst(rmode->fbWidth, n_xfb_lines);
        GX_SetCopyClamp(GX_CLAMP_TOP);
        GX_SetDispCopySrc(0, 0, rmode->fbWidth, lines);
        GX_CopyDisp(buffer, GX_TRUE);
        break;

    case HSD_RP_BOTTOMHALF:
        lines = rmode->efbHeight - HSD_ANTIALIAS_OVERLAP;
        GX_SetDispCopySrc(0, 0, rmode->fbWidth, lines);
        n_xfb_lines = GX_SetDispCopyYScale(1.0f);
        GX_SetDispCopyDst(rmode->fbWidth, n_xfb_lines);
        GX_SetCopyClamp(GX_CLAMP_BOTTOM);
        GX_SetDispCopySrc(0, HSD_ANTIALIAS_OVERLAP, rmode->fbWidth, lines);
        offset = (VIDEO_PadFramebufferWidth(rmode->fbWidth) * lines * (u32)VI_DISPLAY_PIX_SZ);
        GX_CopyDisp((void*)((u32)buffer + offset), GX_TRUE);
        GX_SetDispCopySrc(0, 0, rmode->fbWidth, HSD_ANTIALIAS_OVERLAP);
        GX_SetCopyClamp((GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
        GX_CopyDisp((void*)garbage, GX_TRUE);
        break;

    default:
        HSD_Panic("Unexpected type of render pass.\n");
    }

    GX_PixModeSync();
}

s32 HSD_VIWaitXFBDrawEnable(void)
{
    s32 idx = -1;
    if (HSD_VIGetNbXFB() < 2)
        return idx;
    while ((idx = HSD_VIGetXFBDrawEnable()) == -1)
        VIDEO_WaitVSync();
    return idx;
}

void HSD_VIGXSetDrawDone(s32 arg)
{
    while (HSD_VIGetDrawDoneWaitingFlag())
        GX_WaitDrawDone();
    _p->drawdone.waiting = 1;
    _p->drawdone.arg = arg;
    GX_SetDrawDone();
}

void HSD_VIGXDrawDone(s32 arg)
{
    while (HSD_VIGetDrawDoneWaitingFlag())
        GX_WaitDrawDone();
    _p->drawdone.waiting = 1;
    _p->drawdone.arg = arg;
    GX_DrawDone();
}

//803761C0
void HSD_VICopyXFBASync(HSD_RenderPass rpass)
{
    s32 idx = -1;

    if (HSD_VIGetNbXFB() >= 2) {
        idx = HSD_VIWaitXFBDrawEnable();

        HSD_VICopyEFB2XFBPtr(&_p->efb.vi_all.vi, _p->xfb[idx].buffer, rpass);

        HSD_VISetXFBWaitDone(idx);

        HSD_VIGXSetDrawDone(idx);
    }
}

//803762C4
void HSD_VIDrawDoneXFB(s32 idx)
{
    u32 intr = IRQ_Disable();

    assert(_p->xfb[idx].status == HSD_VI_XFB_WAITDONE);
    _p->xfb[idx].status = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT) != -1 ? HSD_VI_XFB_DRAWDONE : HSD_VI_XFB_NEXT;

    IRQ_Restore(intr);
}

void HSD_VISetXFBWaitDone(s32 idx)
{
    u32 intr;
    intr = IRQ_Disable();
    assert(_p->xfb[idx].status == HSD_VI_XFB_DRAWING);
    _p->xfb[idx].status = HSD_VI_XFB_WAITDONE;
    _p->xfb[idx].vi_all = _p->current;
    _p->current.chg_flag = 0;
    IRQ_Restore(intr);
}

//8037639C
void HSD_VISetXFBDrawDone(void)
{
    u32 intr;
    s32 idx;
    if (HSD_VIGetNbXFB() >= 2) {
        intr = IRQ_Disable();

        idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWING);
        assert(idx != -1);

        HSD_VISetXFBWaitDone(idx);

        HSD_VIDrawDoneXFB(idx);

        IRQ_Restore(intr);
    }
}

//807364A8
s32 HSD_VIGetXFBLastDrawDone(void)
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

//803765A8
void HSD_VISetEFBDrawDone(void)
{
    u32 intr;

    if (HSD_VIGetNbXFB() > 1)
        return;

    HSD_VIGXDrawDone(-1);

    intr = IRQ_Disable();
    _p->efb.vi_all = _p->current;
    _p->current.chg_flag = FALSE;

    _p->efb.status = HSD_VI_EFB_DRAWDONE;
    IRQ_Restore(intr);
}

//80376718
void HSD_VISetConfigure(GXRModeObj* rmode)
{
    _p->current.vi.rmode = *rmode;
    _p->current.chg_flag = TRUE;
}

//803767A0
void HSD_VISetBlack(u8 black)
{
    _p->current.vi.black = black;
    _p->current.chg_flag = TRUE;
}

void Install_ExceptionHandler(void);

//803767B8
void HSD_VIInit(HSD_VIStatus* vi, void* xfb0, void* xfb1, void* xfb2)
{
    s32 idx;

    Install_ExceptionHandler();

    _p->current.vi = *vi;
    _p->current.chg_flag = FALSE;

    _p->xfb[0].buffer = xfb0;
    _p->xfb[1].buffer = xfb1;
    _p->xfb[2].buffer = xfb2;

    for (u32 i = 0; i < HSD_VI_XFB_MAX; i++) {
        _p->xfb[i].vi_all = _p->current;
        if (_p->xfb[i].buffer != NULL) {
            _p->xfb[i].status = HSD_VI_XFB_FREE;
            HSD_VINumXFB += 1;
        } else {
            _p->xfb[i].status = HSD_VI_XFB_NONE;
        }
    }

    _p->nb_xfb = HSD_VINumXFB;

    _p->efb.status = HSD_VI_EFB_FREE;
    _p->efb.vi_all = _p->current;

    VIDEO_SetPreRetraceCallback(HSD_VIPreRetraceCB);
    VIDEO_SetPostRetraceCallback(HSD_VIPostRetraceCB);

    _p->pre_cb = NULL;
    _p->post_cb = NULL;

    _p->drawdone.waiting = 0;
    _p->drawdone.arg = 0;

    GX_SetDrawDoneCallback(HSD_VIGXDrawDoneCB);

    _p->drawdone.cb = NULL;

    _p->perf.frame_period = VIDEO_GetCurrentTvMode() == VI_NTSC ? 60 : 50;
    _p->perf.frame_renew = 0;

    VIDEO_Configure(&_p->current.vi.rmode);
    VIDEO_SetBlack(_p->current.vi.black);
    VIDEO_Flush();

    idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE);
    HSD_VICopyEFB2XFBPtr(&_p->efb.vi_all.vi, _p->xfb[idx].buffer, HSD_RP_SCREEN);
}

extern void __exception_sethandler(u32 nExcept, void (*pHndl)(frame_context*));

//80394848
static void PrintStackTrace(frame_context* pCtx)
{
    printf("- STACK -------------------------------------\n");
    printf(" Address:  Back Chain  LR Save\n");
}

//80394B18
static void PrintExceptionDetails(frame_context* pCtx)
{
    printf("- UNHANDLED EXCEPTION -----------------------\n");
    printf("DAR=%08X\n", pCtx->DAR);
    printf("ERROR %u: ", pCtx->EXCPT_Number);
}

//80228B28
static void ExceptionHandler(frame_context* pCtx)
{
    HSD_VISetUserPreRetraceCallback(NULL);
    HSD_VISetUserPostRetraceCallback(NULL);

    //printf("DATE %s TIME %s", __DATE__, __TIME__);

    //PrintStackTrace(pCtx);
    //PrintExceptionDetails(pCtx);
}

void Install_ExceptionHandler()
{
    __exception_sethandler(EX_SYS_RESET, ExceptionHandler);
    __exception_sethandler(EX_MACH_CHECK, ExceptionHandler);
    __exception_sethandler(EX_DSI, ExceptionHandler);
    __exception_sethandler(EX_ISI, ExceptionHandler);
    __exception_sethandler(EX_ALIGN, ExceptionHandler);
    __exception_sethandler(EX_PRG, ExceptionHandler);
    __exception_sethandler(EX_TRACE, ExceptionHandler);
    __exception_sethandler(EX_PERF, ExceptionHandler);
    __exception_sethandler(EX_IABR, ExceptionHandler);
    __exception_sethandler(EX_RESV, ExceptionHandler);
    __exception_sethandler(EX_THERM, ExceptionHandler);
}