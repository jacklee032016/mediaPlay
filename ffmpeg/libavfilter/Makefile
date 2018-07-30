#
# $Id$
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = . x86

C_FLAGS += $(FFMPEG_CFLAGS)
ASM_FLAGS += -I libavfilter/x86/ 

################################################
####   source file 
################################################
X86_SRC += \
		x86/af_volume.asm  x86/af_volume_init.c  \
		x86/avf_showcqt.asm  x86/avf_showcqt_init.c  \
		x86/colorspacedsp.asm  x86/colorspacedsp_init.c  \
		x86/vf_blend.asm  x86/vf_blend_init.c  \
		x86/vf_bwdif.asm  x86/vf_bwdif_init.c  \
		x86/vf_gradfun.asm  x86/vf_gradfun_init.c  \
		x86/vf_idet.asm  x86/vf_idet_init.c  \
		x86/vf_maskedmerge.asm  x86/vf_maskedmerge_init.c  \
		x86/vf_noise.c  \
		x86/vf_psnr.asm  x86/vf_psnr_init.c  \
		x86/vf_removegrain_init.c  \
		x86/vf_ssim.asm  x86/vf_ssim_init.c  \
		x86/vf_w3fdif.asm  x86/vf_w3fdif_init.c  \
		x86/vf_yadif.asm  x86/vf_yadif_init.c  x86/yadif-10.asm  x86/yadif-16.asm 


SRC += \
		$(X86_SRC) \
		aeval.c  af_acrusher.c  af_adelay.c  af_aecho.c  af_aemphasis.c  af_afade.c  \
		af_afftfilt.c  af_aformat.c  af_agate.c  af_alimiter.c  af_amerge.c  af_amix.c  \
		af_anequalizer.c  af_anull.c  af_apad.c  af_aphaser.c  af_apulsator.c  af_aresample.c  \
		af_asetnsamples.c  af_asetrate.c  af_ashowinfo.c  af_astats.c  af_atempo.c  af_biquads.c  \
		af_channelmap.c  af_channelsplit.c  af_chorus.c  af_compand.c  af_compensationdelay.c  af_crystalizer.c  \
		af_dcshift.c  af_dynaudnorm.c  af_earwax.c  af_extrastereo.c  af_firequalizer.c  af_flanger.c  \
		af_hdcd.c  af_join.c  af_loudnorm.c  af_pan.c  af_replaygain.c  af_sidechaincompress.c  \
		af_silencedetect.c  af_silenceremove.c  af_stereotools.c  af_stereowiden.c  af_tremolo.c  \
		af_vibrato.c  af_volume.c  af_volumedetect.c  allfilters.c  asink_anullsink.c  \
		asrc_anoisesrc.c  asrc_anullsrc.c  asrc_sine.c  audio.c  avf_abitscope.c  \
		avf_ahistogram.c  avf_aphasemeter.c  avf_avectorscope.c  avf_concat.c  \
		avf_showcqt.c  avf_showfreqs.c  avf_showspectrum.c  avf_showvolume.c  \
		avf_showwaves.c  avfilter.c  avfiltergraph.c  bbox.c  buffersink.c  buffersrc.c  \
		colorspacedsp.c  drawutils.c  dualinput.c  ebur128.c  f_bench.c  f_drawgraph.c  \
		f_ebur128.c  f_interleave.c  f_loop.c  f_metadata.c  f_perms.c  f_realtime.c  \
		f_reverse.c  f_select.c  f_sendcmd.c  f_sidedata.c  f_streamselect.c  fifo.c  \
		formats.c  framepool.c  framequeue.c  framesync.c  generate_wave_table.c  graphdump.c  \
		graphparser.c  lavfutils.c  lswsutils.c  motion_estimation.c  opencl_allkernels.c  pthread.c  \
		scale.c  setpts.c  settb.c  split.c  src_movie.c  transform.c  \
		trim.c  vaf_spectrumsynth.c  vf_alphamerge.c  vf_aspect.c  vf_atadenoise.c  \
		vf_avgblur.c  vf_bbox.c  vf_bitplanenoise.c  vf_blackdetect.c  vf_blend.c  \
		vf_bwdif.c  vf_chromakey.c  vf_ciescope.c  vf_codecview.c  vf_colorbalance.c  \
		vf_colorchannelmixer.c  vf_colorkey.c  vf_colorlevels.c  vf_colorspace.c  vf_convolution.c  \
		vf_copy.c  vf_crop.c  vf_curves.c  vf_datascope.c  vf_dctdnoiz.c  vf_deband.c  \
		vf_decimate.c  vf_dejudder.c  vf_deshake.c  vf_detelecine.c  vf_displace.c  \
		vf_drawbox.c  vf_edgedetect.c  vf_elbg.c  vf_extractplanes.c  vf_fade.c  \
		vf_fftfilt.c  vf_field.c  vf_fieldhint.c  vf_fieldmatch.c  vf_fieldorder.c  \
		vf_format.c  vf_fps.c  vf_framepack.c  vf_framerate.c  vf_framestep.c  \
		vf_gblur.c  vf_gradfun.c  vf_hflip.c  vf_histogram.c  vf_hqx.c  vf_hue.c  \
		vf_hwdownload.c  vf_hwmap.c  vf_hwupload.c  vf_hwupload_cuda.c  vf_hysteresis.c  \
		vf_idet.c  vf_il.c  vf_lenscorrection.c  vf_lut.c  vf_lut2.c  vf_lut3d.c  \
		vf_maskedclamp.c  vf_maskedmerge.c  vf_mergeplanes.c  vf_mestimate.c  vf_midequalizer.c  \
		vf_minterpolate.c  vf_neighbor.c  vf_nlmeans.c  vf_noise.c  vf_null.c  vf_overlay.c  \
		vf_pad.c  vf_palettegen.c  vf_paletteuse.c  vf_pixdesctest.c  vf_premultiply.c  vf_psnr.c  \
		vf_qp.c  vf_random.c  vf_readeia608.c  vf_readvitc.c  vf_remap.c  vf_removegrain.c  \
		vf_removelogo.c  vf_rotate.c  vf_scale.c  vf_selectivecolor.c  vf_separatefields.c  \
		vf_setfield.c  vf_showinfo.c  vf_showpalette.c  vf_shuffleframes.c  vf_shuffleplanes.c  \
		vf_signalstats.c  vf_ssim.c  vf_stack.c  vf_swaprect.c  vf_swapuv.c  vf_telecine.c  \
		vf_threshold.c  vf_thumbnail.c  vf_tile.c  vf_transpose.c  vf_unsharp.c  vf_vectorscope.c  \
		vf_vflip.c  vf_vignette.c  vf_w3fdif.c  vf_waveform.c  vf_weave.c  vf_xbr.c  vf_yadif.c  \
		vf_zoompan.c  video.c  vsink_nullsink.c  vsrc_cellauto.c  vsrc_life.c  vsrc_mandelbrot.c  \
		vsrc_testsrc.c  window_func.c  



TARGET_LIB:=libavfilter

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

