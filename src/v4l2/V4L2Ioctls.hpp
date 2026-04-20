#ifndef CEMU_CAPTURE_V4L2_IOCTL_FUNCS_HPP
#define CEMU_CAPTURE_V4L2_IOCTL_FUNCS_HPP


#include <linux/videodev2.h>
#include <sys/ioctl.h>
#if defined(__clang__) || !defined(__GNUC__)
#define V4L2_R
#define V4L2_RW
#define V4L2_W
#elif defined(__GNUC__)
#define V4L2_R __attribute__((access(write_only, 2)))
#define V4L2_RW __attribute__((access(read_write, 2)))
#define V4L2_W __attribute__((access(read_only, 2)))
#endif
#define V4L2_ARG_R
#define V4L2_ARG_RW
#define V4L2_ARG_W
namespace cemu_capture::vidioc
{
V4L2_R inline int querycap (FileDescriptor& fd, V4L2_ARG_R v4l2_capability* var) { return fd.XIoctl(VIDIOC_QUERYCAP, var); }
V4L2_RW inline int enum_fmt (FileDescriptor& fd, V4L2_ARG_RW v4l2_fmtdesc* var) { return fd.XIoctl(VIDIOC_ENUM_FMT, var); }
V4L2_RW inline int g_fmt (FileDescriptor& fd, V4L2_ARG_RW v4l2_format* var) { return fd.XIoctl(VIDIOC_G_FMT, var); }
V4L2_RW inline int s_fmt (FileDescriptor& fd, V4L2_ARG_RW v4l2_format* var) { return fd.XIoctl(VIDIOC_S_FMT, var); }
V4L2_RW inline int reqbufs (FileDescriptor& fd, V4L2_ARG_RW v4l2_requestbuffers* var) { return fd.XIoctl(VIDIOC_REQBUFS, var); }
V4L2_RW inline int querybuf (FileDescriptor& fd, V4L2_ARG_RW v4l2_buffer* var) { return fd.XIoctl(VIDIOC_QUERYBUF, var); }
V4L2_R inline int g_fbuf (FileDescriptor& fd, V4L2_ARG_R v4l2_framebuffer* var) { return fd.XIoctl(VIDIOC_G_FBUF, var); }
V4L2_W inline int s_fbuf (FileDescriptor& fd, V4L2_ARG_W const v4l2_framebuffer* var) { return fd.XIoctl(VIDIOC_S_FBUF, (v4l2_framebuffer*)var); }
V4L2_W inline int overlay (FileDescriptor& fd, V4L2_ARG_W const int* var) { return fd.XIoctl(VIDIOC_OVERLAY, (int*)var); }
V4L2_RW inline int qbuf (FileDescriptor& fd, V4L2_ARG_RW v4l2_buffer* var) { return fd.XIoctl(VIDIOC_QBUF, var); }
V4L2_RW inline int expbuf (FileDescriptor& fd, V4L2_ARG_RW v4l2_exportbuffer* var) { return fd.XIoctl(VIDIOC_EXPBUF, var); }
V4L2_RW inline int dqbuf (FileDescriptor& fd, V4L2_ARG_RW v4l2_buffer* var) { return fd.XIoctl(VIDIOC_DQBUF, var); }
V4L2_W inline int streamon (FileDescriptor& fd, V4L2_ARG_W const int* var) { return fd.XIoctl(VIDIOC_STREAMON, (int*)var); }
V4L2_W inline int streamoff (FileDescriptor& fd, V4L2_ARG_W const int* var) { return fd.XIoctl(VIDIOC_STREAMOFF, (int*)var); }
V4L2_RW inline int g_parm (FileDescriptor& fd, V4L2_ARG_RW v4l2_streamparm* var) { return fd.XIoctl(VIDIOC_G_PARM, var); }
V4L2_RW inline int s_parm (FileDescriptor& fd, V4L2_ARG_RW v4l2_streamparm* var) { return fd.XIoctl(VIDIOC_S_PARM, var); }
V4L2_R inline int g_std (FileDescriptor& fd, V4L2_ARG_R v4l2_std_id* var) { return fd.XIoctl(VIDIOC_G_STD, var); }
V4L2_W inline int s_std (FileDescriptor& fd, V4L2_ARG_W const v4l2_std_id* var) { return fd.XIoctl(VIDIOC_S_STD, (v4l2_std_id*)var); }
V4L2_RW inline int enumstd (FileDescriptor& fd, V4L2_ARG_RW v4l2_standard* var) { return fd.XIoctl(VIDIOC_ENUMSTD, var); }
V4L2_RW inline int enuminput (FileDescriptor& fd, V4L2_ARG_RW v4l2_input* var) { return fd.XIoctl(VIDIOC_ENUMINPUT, var); }
V4L2_RW inline int g_ctrl (FileDescriptor& fd, V4L2_ARG_RW v4l2_control* var) { return fd.XIoctl(VIDIOC_G_CTRL, var); }
V4L2_RW inline int s_ctrl (FileDescriptor& fd, V4L2_ARG_RW v4l2_control* var) { return fd.XIoctl(VIDIOC_S_CTRL, var); }
V4L2_RW inline int g_tuner (FileDescriptor& fd, V4L2_ARG_RW v4l2_tuner* var) { return fd.XIoctl(VIDIOC_G_TUNER, var); }
V4L2_W inline int s_tuner (FileDescriptor& fd, V4L2_ARG_W const v4l2_tuner* var) { return fd.XIoctl(VIDIOC_S_TUNER, (v4l2_tuner*)var); }
V4L2_R inline int g_audio (FileDescriptor& fd, V4L2_ARG_R v4l2_audio* var) { return fd.XIoctl(VIDIOC_G_AUDIO, var); }
V4L2_W inline int s_audio (FileDescriptor& fd, V4L2_ARG_W const v4l2_audio* var) { return fd.XIoctl(VIDIOC_S_AUDIO, (v4l2_audio*)var); }
V4L2_RW inline int queryctrl (FileDescriptor& fd, V4L2_ARG_RW v4l2_queryctrl* var) { return fd.XIoctl(VIDIOC_QUERYCTRL, var); }
V4L2_RW inline int querymenu (FileDescriptor& fd, V4L2_ARG_RW v4l2_querymenu* var) { return fd.XIoctl(VIDIOC_QUERYMENU, var); }
V4L2_R inline int g_input (FileDescriptor& fd, V4L2_ARG_R int* var) { return fd.XIoctl(VIDIOC_G_INPUT, var); }
V4L2_RW inline int s_input (FileDescriptor& fd, V4L2_ARG_RW int* var) { return fd.XIoctl(VIDIOC_S_INPUT, var); }
V4L2_RW inline int g_edid (FileDescriptor& fd, V4L2_ARG_RW v4l2_edid* var) { return fd.XIoctl(VIDIOC_G_EDID, var); }
V4L2_RW inline int s_edid (FileDescriptor& fd, V4L2_ARG_RW v4l2_edid* var) { return fd.XIoctl(VIDIOC_S_EDID, var); }
V4L2_R inline int g_output (FileDescriptor& fd, V4L2_ARG_R int* var) { return fd.XIoctl(VIDIOC_G_OUTPUT, var); }
V4L2_RW inline int s_output (FileDescriptor& fd, V4L2_ARG_RW int* var) { return fd.XIoctl(VIDIOC_S_OUTPUT, var); }
V4L2_RW inline int enumoutput (FileDescriptor& fd, V4L2_ARG_RW v4l2_output* var) { return fd.XIoctl(VIDIOC_ENUMOUTPUT, var); }
V4L2_R inline int g_audout (FileDescriptor& fd, V4L2_ARG_R v4l2_audioout* var) { return fd.XIoctl(VIDIOC_G_AUDOUT, var); }
V4L2_W inline int s_audout (FileDescriptor& fd, V4L2_ARG_W const v4l2_audioout* var) { return fd.XIoctl(VIDIOC_S_AUDOUT, (v4l2_audioout*)var); }
V4L2_RW inline int g_modulator (FileDescriptor& fd, V4L2_ARG_RW v4l2_modulator* var) { return fd.XIoctl(VIDIOC_G_MODULATOR, var); }
V4L2_W inline int s_modulator (FileDescriptor& fd, V4L2_ARG_W const v4l2_modulator* var) { return fd.XIoctl(VIDIOC_S_MODULATOR, (v4l2_modulator*)var); }
V4L2_RW inline int g_frequency (FileDescriptor& fd, V4L2_ARG_RW v4l2_frequency* var) { return fd.XIoctl(VIDIOC_G_FREQUENCY, var); }
V4L2_W inline int s_frequency (FileDescriptor& fd, V4L2_ARG_W const v4l2_frequency* var) { return fd.XIoctl(VIDIOC_S_FREQUENCY, (v4l2_frequency*)var); }
V4L2_RW inline int cropcap (FileDescriptor& fd, V4L2_ARG_RW v4l2_cropcap* var) { return fd.XIoctl(VIDIOC_CROPCAP, var); }
V4L2_RW inline int g_crop (FileDescriptor& fd, V4L2_ARG_RW v4l2_crop* var) { return fd.XIoctl(VIDIOC_G_CROP, var); }
V4L2_W inline int s_crop (FileDescriptor& fd, V4L2_ARG_W const v4l2_crop* var) { return fd.XIoctl(VIDIOC_S_CROP, (v4l2_crop*)var); }
V4L2_R inline int g_jpegcomp (FileDescriptor& fd, V4L2_ARG_R v4l2_jpegcompression* var) { return fd.XIoctl(VIDIOC_G_JPEGCOMP, var); }
V4L2_W inline int s_jpegcomp (FileDescriptor& fd, V4L2_ARG_W const v4l2_jpegcompression* var) { return fd.XIoctl(VIDIOC_S_JPEGCOMP, (v4l2_jpegcompression*)var); }
V4L2_R inline int querystd (FileDescriptor& fd, V4L2_ARG_R v4l2_std_id* var) { return fd.XIoctl(VIDIOC_QUERYSTD, var); }
V4L2_RW inline int try_fmt (FileDescriptor& fd, V4L2_ARG_RW v4l2_format* var) { return fd.XIoctl(VIDIOC_TRY_FMT, var); }
V4L2_RW inline int enumaudio (FileDescriptor& fd, V4L2_ARG_RW v4l2_audio* var) { return fd.XIoctl(VIDIOC_ENUMAUDIO, var); }
V4L2_RW inline int enumaudout (FileDescriptor& fd, V4L2_ARG_RW v4l2_audioout* var) { return fd.XIoctl(VIDIOC_ENUMAUDOUT, var); }
V4L2_R inline int g_priority (FileDescriptor& fd, V4L2_ARG_R __u32* var) { return fd.XIoctl(VIDIOC_G_PRIORITY, var); }
V4L2_W inline int s_priority (FileDescriptor& fd, V4L2_ARG_W const __u32* var) { return fd.XIoctl(VIDIOC_S_PRIORITY, (__u32*)var); }
V4L2_RW inline int g_sliced_vbi_cap (FileDescriptor& fd, V4L2_ARG_RW v4l2_sliced_vbi_cap* var) { return fd.XIoctl(VIDIOC_G_SLICED_VBI_CAP, var); }
inline int log_status (FileDescriptor& fd) { return fd.XIoctl(VIDIOC_LOG_STATUS); }
V4L2_RW inline int g_ext_ctrls (FileDescriptor& fd, V4L2_ARG_RW v4l2_ext_controls* var) { return fd.XIoctl(VIDIOC_G_EXT_CTRLS, var); }
V4L2_RW inline int s_ext_ctrls (FileDescriptor& fd, V4L2_ARG_RW v4l2_ext_controls* var) { return fd.XIoctl(VIDIOC_S_EXT_CTRLS, var); }
V4L2_RW inline int try_ext_ctrls (FileDescriptor& fd, V4L2_ARG_RW v4l2_ext_controls* var) { return fd.XIoctl(VIDIOC_TRY_EXT_CTRLS, var); }
V4L2_RW inline int enum_framesizes (FileDescriptor& fd, V4L2_ARG_RW v4l2_frmsizeenum* var) { return fd.XIoctl(VIDIOC_ENUM_FRAMESIZES, var); }
V4L2_RW inline int enum_frameintervals (FileDescriptor& fd, V4L2_ARG_RW v4l2_frmivalenum* var) { return fd.XIoctl(VIDIOC_ENUM_FRAMEINTERVALS, var); }
V4L2_R inline int g_enc_index (FileDescriptor& fd, V4L2_ARG_R v4l2_enc_idx* var) { return fd.XIoctl(VIDIOC_G_ENC_INDEX, var); }
V4L2_RW inline int encoder_cmd (FileDescriptor& fd, V4L2_ARG_RW v4l2_encoder_cmd* var) { return fd.XIoctl(VIDIOC_ENCODER_CMD, var); }
V4L2_RW inline int try_encoder_cmd (FileDescriptor& fd, V4L2_ARG_RW v4l2_encoder_cmd* var) { return fd.XIoctl(VIDIOC_TRY_ENCODER_CMD, var); }
V4L2_W inline int dbg_s_register (FileDescriptor& fd, V4L2_ARG_W const v4l2_dbg_register* var) { return fd.XIoctl(VIDIOC_DBG_S_REGISTER, (v4l2_dbg_register*)var); }
V4L2_RW inline int dbg_g_register (FileDescriptor& fd, V4L2_ARG_RW v4l2_dbg_register* var) { return fd.XIoctl(VIDIOC_DBG_G_REGISTER, var); }
V4L2_W inline int s_hw_freq_seek (FileDescriptor& fd, V4L2_ARG_W const v4l2_hw_freq_seek* var) { return fd.XIoctl(VIDIOC_S_HW_FREQ_SEEK, (v4l2_hw_freq_seek*)var); }
V4L2_RW inline int s_dv_timings (FileDescriptor& fd, V4L2_ARG_RW v4l2_dv_timings* var) { return fd.XIoctl(VIDIOC_S_DV_TIMINGS, var); }
V4L2_RW inline int g_dv_timings (FileDescriptor& fd, V4L2_ARG_RW v4l2_dv_timings* var) { return fd.XIoctl(VIDIOC_G_DV_TIMINGS, var); }
V4L2_R inline int dqevent (FileDescriptor& fd, V4L2_ARG_R v4l2_event* var) { return fd.XIoctl(VIDIOC_DQEVENT, var); }
V4L2_W inline int subscribe_event (FileDescriptor& fd, V4L2_ARG_W const v4l2_event_subscription* var) { return fd.XIoctl(VIDIOC_SUBSCRIBE_EVENT, (v4l2_event_subscription*)var); }
V4L2_W inline int unsubscribe_event (FileDescriptor& fd, V4L2_ARG_W const v4l2_event_subscription* var) { return fd.XIoctl(VIDIOC_UNSUBSCRIBE_EVENT, (v4l2_event_subscription*)var); }
V4L2_RW inline int create_bufs (FileDescriptor& fd, V4L2_ARG_RW v4l2_create_buffers* var) { return fd.XIoctl(VIDIOC_CREATE_BUFS, var); }
V4L2_RW inline int prepare_buf (FileDescriptor& fd, V4L2_ARG_RW v4l2_buffer* var) { return fd.XIoctl(VIDIOC_PREPARE_BUF, var); }
V4L2_RW inline int g_selection (FileDescriptor& fd, V4L2_ARG_RW v4l2_selection* var) { return fd.XIoctl(VIDIOC_G_SELECTION, var); }
V4L2_RW inline int s_selection (FileDescriptor& fd, V4L2_ARG_RW v4l2_selection* var) { return fd.XIoctl(VIDIOC_S_SELECTION, var); }
V4L2_RW inline int decoder_cmd (FileDescriptor& fd, V4L2_ARG_RW v4l2_decoder_cmd* var) { return fd.XIoctl(VIDIOC_DECODER_CMD, var); }
V4L2_RW inline int try_decoder_cmd (FileDescriptor& fd, V4L2_ARG_RW v4l2_decoder_cmd* var) { return fd.XIoctl(VIDIOC_TRY_DECODER_CMD, var); }
V4L2_RW inline int enum_dv_timings (FileDescriptor& fd, V4L2_ARG_RW v4l2_enum_dv_timings* var) { return fd.XIoctl(VIDIOC_ENUM_DV_TIMINGS, var); }
V4L2_R inline int query_dv_timings (FileDescriptor& fd, V4L2_ARG_R v4l2_dv_timings* var) { return fd.XIoctl(VIDIOC_QUERY_DV_TIMINGS, var); }
V4L2_RW inline int dv_timings_cap (FileDescriptor& fd, V4L2_ARG_RW v4l2_dv_timings_cap* var) { return fd.XIoctl(VIDIOC_DV_TIMINGS_CAP, var); }
V4L2_RW inline int enum_freq_bands (FileDescriptor& fd, V4L2_ARG_RW v4l2_frequency_band* var) { return fd.XIoctl(VIDIOC_ENUM_FREQ_BANDS, var); }
V4L2_RW inline int dbg_g_chip_info (FileDescriptor& fd, V4L2_ARG_RW v4l2_dbg_chip_info* var) { return fd.XIoctl(VIDIOC_DBG_G_CHIP_INFO, var); }
V4L2_RW inline int query_ext_ctrl (FileDescriptor& fd, V4L2_ARG_RW v4l2_query_ext_ctrl* var) { return fd.XIoctl(VIDIOC_QUERY_EXT_CTRL, var); }
V4L2_RW inline int remove_bufs (FileDescriptor& fd, V4L2_ARG_RW v4l2_remove_buffers* var) { return fd.XIoctl(VIDIOC_REMOVE_BUFS, var); }

}
#endif