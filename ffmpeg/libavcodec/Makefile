#
# $Id$
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = . x86

C_FLAGS +=  $(FFMPEG_CFLAGS)

ifeq ($(ARCH),arm)
	ASM_FLAGS += -I libavfilter/x86/ 
else
	ASM_FLAGS += -I libavfilter/x86/ 
endif

################################################
####   source file 
################################################
ifeq ($(ARCH),arm)
ARCH_SRC += \

else
ARCH_SRC += \
		x86/aacencdsp.asm x86/aacencdsp_init.c \
		x86/aacpsdsp.asm x86/aacpsdsp_init.c \
		x86/ac3dsp.asm x86/ac3dsp_downmix.asm x86/ac3dsp_init.c \
		x86/alacdsp.asm x86/alacdsp_init.c \
		x86/audiodsp.asm x86/audiodsp_init.c \
		x86/blockdsp.asm x86/blockdsp_init.c \
		x86/bswapdsp.asm x86/bswapdsp_init.c \
		x86/cavsdsp.c x86/constants.c \
		x86/dcadsp.asm x86/dcadsp_init.c \
		x86/dct32.asm x86/dct_init.c \
		x86/dirac_dwt.asm x86/dirac_dwt_init.c \
		x86/diracdsp.asm x86/diracdsp_init.c \
		x86/dnxhdenc.asm x86/dnxhdenc_init.c \
		x86/fdct.c x86/fdctdsp_init.c \
		x86/fft.asm x86/fft_init.c \
		x86/flacdsp.asm x86/flacdsp_init.c \
		x86/fmtconvert.asm x86/fmtconvert_init.c \
		x86/fpel.asm \
		x86/g722dsp.asm x86/g722dsp_init.c \
		x86/h263_loopfilter.asm x86/h263dsp_init.c \
		x86/h264_chromamc.asm x86/h264_chromamc_10bit.asm x86/h264_deblock.asm x86/h264_deblock_10bit.asm x86/h264_idct.asm \
		x86/h264_idct_10bit.asm x86/h264_intrapred.asm x86/h264_intrapred_10bit.c x86/h264_intrapred_init.c \
		x86/h264_qpel.c x86/h264_qpel_10bit.asm x86/h264_qpel_8bit.asm x86/h264_weight.asm x86/h264_weight_10bit.asm \
		x86/h264chroma_init.c x86/h264dsp_init.c \
		x86/hevc_add_res.asm x86/hevc_deblock.asm x86/hevc_idct.asm \
		x86/hevc_mc.asm x86/hevc_sao.asm x86/hevc_sao_10bit.asm x86/hevcdsp_init.c \
		x86/hpeldsp.asm x86/hpeldsp_init.c \
		x86/hpeldsp_vp3.asm x86/hpeldsp_vp3_init.c \
		x86/huffyuvdsp.asm x86/huffyuvdsp_init.c \
		x86/huffyuvencdsp.asm x86/huffyuvencdsp_init.c \
		x86/idctdsp.asm x86/idctdsp_init.c \
		x86/imdct36.asm \
		x86/jpeg2000dsp.asm x86/jpeg2000dsp_init.c \
		x86/lossless_audiodsp.asm x86/lossless_audiodsp_init.c \
		x86/lossless_videodsp.asm x86/lossless_videodsp_init.c \
		x86/lossless_videoencdsp.asm x86/lossless_videoencdsp_init.c \
		x86/lpc.c \
		x86/me_cmp.asm x86/me_cmp_init.c \
		x86/mlpdsp.asm x86/mlpdsp_init.c \
		x86/mpegaudiodsp.c \
		x86/mpegvideo.c x86/mpegvideodsp.c x86/mpegvideoenc.c \
		x86/mpegvideoencdsp.asm x86/mpegvideoencdsp_init.c \
		x86/pixblockdsp.asm x86/pixblockdsp_init.c \
		x86/pngdsp.asm x86/pngdsp_init.c \
		x86/proresdsp.asm x86/proresdsp_init.c \
		x86/qpel.asm x86/qpeldsp.asm x86/qpeldsp_init.c \
		x86/rv34dsp.asm x86/rv34dsp_init.c \
		x86/rv40dsp.asm x86/rv40dsp_init.c \
		x86/sbrdsp.asm x86/sbrdsp_init.c \
		x86/simple_idct.c x86/simple_idct10.asm \
		x86/snowdsp.c \
		x86/svq1enc.asm x86/svq1enc_init.c \
		x86/synth_filter.asm x86/synth_filter_init.c \
		x86/takdsp.asm x86/takdsp_init.c \
		x86/ttadsp.asm x86/ttadsp_init.c \
		x86/ttaencdsp.asm x86/ttaencdsp_init.c \
		x86/v210-init.c x86/v210.asm \
		x86/v210enc.asm x86/v210enc_init.c \
		x86/vc1dsp_init.c x86/vc1dsp_loopfilter.asm x86/vc1dsp_mc.asm x86/vc1dsp_mmx.c \
		x86/videodsp.asm x86/videodsp_init.c \
		x86/vorbisdsp.asm x86/vorbisdsp_init.c \
		x86/vp3dsp.asm x86/vp3dsp_init.c \
		x86/vp6dsp.asm x86/vp6dsp_init.c \
		x86/vp8dsp.asm x86/vp8dsp_init.c x86/vp8dsp_loopfilter.asm \
		x86/vp9dsp_init.c x86/vp9dsp_init_10bpp.c x86/vp9dsp_init_12bpp.c x86/vp9dsp_init_16bpp.c \
		x86/vp9intrapred.asm x86/vp9intrapred_16bpp.asm \
		x86/vp9itxfm.asm x86/vp9itxfm_16bpp.asm \
		x86/vp9lpf.asm x86/vp9lpf_16bpp.asm x86/vp9mc.asm x86/vp9mc_16bpp.asm \
		x86/xvididct.asm x86/xvididct_init.c 


# last build parts
SRC2 += \
		xan.c xbmdec.c xbmenc.c xface.c xfacedec.c xfaceenc.c \
		xl.c xma_parser.c xpmdec.c xsubdec.c xsubenc.c xvididct.c xwddec.c \
		xwdenc.c xxan.c y41pdec.c y41penc.c ylc.c yop.c yuv4dec.c \
		yuv4enc.c zerocodec.c zmbv.c zmbvenc.c \



SRC += \
		$(ARCH_SRC) \
		$(SRC2) \
		012v.c 4xm.c 8bps.c 8svx.c \
		a64multienc.c \
		aac_adtstoasc_bsf.c aac_parser.c \
		aaccoder.c aacdec.c \
		aacdec_fixed.c aacenc.c aacenc_is.c aacenc_ltp.c aacenc_pred.c \
		aacenc_tns.c aacenctab.c aacps_fixed.c aacps_float.c aacpsdsp_fixed.c \
		aacpsdsp_float.c aacpsy.c aacsbr.c aacsbr_fixed.c aactab.c \
		aandcttab.c aasc.c \
		ac3.c \
		ac3dec_data.c ac3dec_fixed.c \
		ac3dec_float.c ac3dsp.c ac3enc.c ac3enc_fixed.c ac3enc_float.c \
		acelp_filters.c acelp_pitch_delay.c acelp_vectors.c adpcm.c adpcm_data.c \
		adpcmenc.c adx.c adx_parser.c adxdec.c adxenc.c aic.c \
		alac.c alac_data.c alacdsp.c alacenc.c aliaspixdec.c aliaspixenc.c \
		alsdec.c amrnbdec.c amrwbdec.c anm.c ansi.c \
		apedec.c ass.c ass_split.c assdec.c assenc.c asv.c \
		asvdec.c asvenc.c atrac.c atrac1.c atrac3.c atrac3plus.c \
		atrac3plusdec.c atrac3plusdsp.c audio_frame_queue.c audioconvert.c audiodsp.c \
		aura.c avdct.c avfft.c \
		avpicture.c \
		avrndec.c \
		avs.c avuidec.c avuienc.c \
		bethsoftvideo.c bfi.c bgmc.c \
		bink.c binkaudio.c binkdsp.c bintext.c \
		bitstream_filters.c \
		blockdsp.c bmp.c bmp_parser.c bmpenc.c bmvaudio.c \
		bmvvideo.c brenderpix.c \
		bswapdsp.c \
		c93.c \
		cabac.c \
		canopus.c \
		cavs.c cavs_parser.c cavsdata.c cavsdec.c cavsdsp.c cbrt_data.c \
		cbrt_data_fixed.c ccaption_dec.c cdgraphics.c cdxl.c celp_filters.c \
		celp_math.c cfhd.c cfhddata.c cga_data.c \
		cinepak.c \
		cinepakenc.c clearvideo.c cljrdec.c cljrenc.c cllc.c cngdec.c \
		cngenc.c \
		cook.c cook_parser.c cpia.c cscd.c \
		cuvid.c cyuv.c \
		d3d11va.c \
		dca.c dca_core.c \
		dca_exss.c dca_lbr.c dca_parser.c dca_xll.c dcadata.c dcadct.c \
		dcadec.c dcadsp.c dcaenc.c dcahuff.c dct.c dct32_fixed.c \
		dct32_float.c dds.c dfa.c dirac.c dirac_arith.c dirac_dwt.c \
		dirac_parser.c dirac_vlc.c diracdec.c diracdsp.c diractab.c \
		dnxhd_parser.c dnxhddata.c dnxhddec.c dnxhdenc.c dpcm.c dpx.c \
		dpx_parser.c dpxenc.c dsd.c dsddec.c dsicinaudio.c dsicinvideo.c \
		dss_sp.c dstdec.c \
		dv.c dv_profile.c dvaudio_parser.c \
		dvaudiodec.c dvbsub.c dvbsub_parser.c dvbsubdec.c dvd_nav_parser.c dvdata.c \
		dvdec.c dvdsub_parser.c dvdsubdec.c dvdsubenc.c dvenc.c dxa.c \
		dxtory.c dxv.c \
		eac3_data.c \
		eac3enc.c eacmv.c eaidct.c \
		eamad.c eatgq.c eatgv.c eatqi.c elbg.c elsdec.c \
		error_resilience.c escape124.c escape130.c evrcdec.c exr.c \
		faandct.c \
		faanidct.c faxcompr.c fdctdsp.c \
		fft_fixed.c fft_fixed_32.c fft_float.c fft_init_table.c ffv1.c \
		ffv1dec.c ffv1enc.c ffwavesynth.c fic.c flac.c flac_parser.c \
		flacdata.c flacdec.c flacdsp.c flacenc.c flashsv.c flashsv2enc.c \
		flashsvenc.c flicvideo.c flvdec.c flvenc.c fmtconvert.c fmvc.c \
		fraps.c \
		frwu.c \
		g2meet.c \
		g722.c g722dec.c \
		g722dsp.c g722enc.c g723_1.c g723_1dec.c g723_1enc.c g726.c \
		g729_parser.c g729dec.c g729postfilter.c gif.c gifdec.c \
		gsm_parser.c gsmdec.c gsmdec_data.c \
		h261.c \
		h261_parser.c \
		h261data.c \
		h261dec.c h261enc.c h263.c h263_parser.c h263data.c h263dec.c \
		h263dsp.c \
		h264_cabac.c h264_cavlc.c h264_direct.c \
		h264_loopfilter.c h264_mb.c \
		h264_parse.c h264_parser.c \
		h264_picture.c h264_ps.c h264_refs.c h264_sei.c h264_slice.c \
		h264chroma.c h264data.c h264dec.c h264dsp.c h264idct.c \
		h264pred.c h264qpel.c hap.c hapdec.c hevc_cabac.c hevc_data.c \
		hevc_filter.c \
		hevc_mvs.c hevc_parser.c hevc_ps.c \
		hevc_refs.c hevc_sei.c hevcdec.c hevcdsp.c hevcpred.c hnm4video.c \
		hpeldsp.c hq_hqa.c hq_hqadata.c hq_hqadsp.c hqx.c hqxdsp.c \
		hqxvlc.c htmlsubtitles.c huffman.c huffyuv.c huffyuvdec.c huffyuvdsp.c \
		huffyuvenc.c huffyuvencdsp.c \
		idcinvideo.c \
		idctdsp.c iff.c iirfilter.c \
		imc.c imgconvert.c \
		indeo2.c indeo3.c indeo4.c \
		indeo5.c intelh263dec.c interplayacm.c interplayvideo.c intrax8.c intrax8dsp.c \
		ituh263dec.c ituh263enc.c ivi.c ivi_dsp.c \
		j2kenc.c \
		jacosubdec.c \
		jfdctfst.c jfdctint.c jni.c jpeg2000.c jpeg2000dec.c jpeg2000dsp.c \
		jpeg2000dwt.c jpegls.c jpeglsdec.c jpeglsenc.c \
		jrevdct.c \
		jvdec.c \
		kbdwin.c \
		kgv1dec.c kmvc.c \
		lagarith.c \
		lagarithrac.c \
		latm_parser.c \
		lcldec.c lclenc.c ljpegenc.c loco.c lossless_audiodsp.c lossless_videodsp.c \
		lossless_videoencdsp.c lpc.c lsp.c lzf.c lzw.c lzwenc.c \
		m101.c \
		mace.c \
		magicyuv.c mathtables.c mdct15.c mdct_fixed.c mdct_fixed_32.c \
		mdct_float.c mdec.c \
		me_cmp.c \
		mediacodec.c metasound.c metasound_data.c \
		microdvddec.c mimic.c \
		mjpeg_parser.c \
		mjpegbdec.c mjpegdec.c mjpegenc.c mjpegenc_common.c mjpegenc_huffman.c mlp.c \
		mlp_parser.c mlpdec.c mlpdsp.c mlpenc.c mlz.c mmvideo.c motion_est.c \
		motionpixels.c \
		movtextdec.c movtextenc.c \
		mpc.c mpc7.c mpc8.c mpeg12.c mpeg12data.c mpeg12dec.c mpeg12enc.c \
		mpeg12framerate.c \
		mpeg4video.c mpeg4video_parser.c \
		mpeg4videodec.c mpeg4videoenc.c mpeg_er.c mpegaudio.c mpegaudio_parser.c \
		mpegaudiodec_fixed.c mpegaudiodec_float.c mpegaudiodsp.c mpegaudiodsp_data.c \
		mpegaudiodsp_fixed.c mpegaudiodsp_float.c mpegaudioenc_fixed.c mpegaudioenc_float.c mpegpicture.c \
		mpegutils.c mpegvideo.c mpegvideo_enc.c mpegvideo_motion.c mpegvideo_parser.c mpegvideodata.c \
		mpegvideodsp.c mpegvideoencdsp.c mpl2dec.c mqc.c mqcdec.c mqcenc.c msgsmdec.c \
		msmpeg4.c msmpeg4data.c msmpeg4dec.c msmpeg4enc.c msrle.c msrledec.c mss1.c \
		mss12.c mss2.c mss2dsp.c mss3.c mss34dsp.c mss4.c msvideo1.c msvideo1enc.c \
		mvcdec.c mxpegdec.c \
		nellymoser.c \
		nellymoserdec.c \
		nellymoserenc.c \
		nuv.c \
		nvenc.c nvenc_h264.c nvenc_hevc.c \
		on2avc.c \
		on2avcdata.c \
		opus.c \
		opus_celt.c opus_parser.c opus_pvq.c opus_rc.c opus_silk.c opusdec.c opusenc.c \
		opustab.c \
		pafaudio.c \
		pafvideo.c pamenc.c \
		pcm-bluray.c pcm-dvd.c \
		pcm.c pcx.c pcxenc.c pgssubdec.c pictordec.c pixblockdsp.c pixlet.c \
		png.c png_parser.c pngdec.c pngdsp.c pngenc.c pnm.c pnm_parser.c \
		pnmdec.c pnmenc.c \
		proresdata.c proresdec2.c proresdec_lgpl.c \
		proresdsp.c proresenc_anatoliy.c proresenc_kostya.c psd.c psymodel.c \
		ptx.c \
		qcelpdec.c \
		qdm2.c \
		qdmc.c \
		qdrw.c \
		qpeg.c qpeldsp.c qsv_api.c qtrle.c qtrleenc.c \
		r210dec.c \
		r210enc.c \
		ra144.c ra144dec.c ra144enc.c ra288.c ralf.c rangecoder.c ratecontrol.c \
		raw.c \
		rawdec.c \
		rawenc.c \
		rdft.c realtextdec.c \
		resample.c \
		resample2.c rl.c rl2.c rle.c roqaudioenc.c roqvideo.c roqvideodec.c \
		roqvideoenc.c rpza.c rscc.c rtjpeg.c rv10.c rv10enc.c rv20enc.c \
		rv30.c rv30dsp.c rv34.c rv34_parser.c rv34dsp.c rv40.c rv40dsp.c \
		s302m.c \
		s302menc.c \
		samidec.c sanm.c sbrdsp.c sbrdsp_fixed.c scpr.c \
		screenpresso.c sgidec.c sgienc.c sgirledec.c sheervideo.c shorten.c \
		sinewin.c sinewin_fixed.c sipr.c sipr16k.c sipr_parser.c \
		smacker.c smc.c smvjpegdec.c snappy.c snow.c snow_dwt.c snowdec.c \
		snowenc.c sonic.c sp5xdec.c speedhq.c srtdec.c srtenc.c startcode.c \
		subviewerdec.c sunrast.c sunrastenc.c svq1.c svq13.c svq1dec.c svq1enc.c \
		svq3.c synth_filter.c \
		tak.c \
		tak_parser.c \
		takdec.c takdsp.c targa.c \
		targa_y216dec.c targaenc.c tdsc.c textdec.c texturedsp.c tiertexseqv.c \
		tiff.c tiff_data.c tiffenc.c tmv.c tpeldsp.c \
		truemotion1.c truemotion2.c truemotion2rt.c truespeech.c tscc.c \
		tscc2.c tta.c ttadata.c ttadsp.c ttaenc.c ttaencdsp.c \
		twinvq.c twinvqdec.c txd.c ulti.c \
		utvideo.c \
		utvideodec.c \
		utvideoenc.c \
		v210dec.c \
		v210enc.c \
		v210x.c v308dec.c v308enc.c \
		v408dec.c v408enc.c v410dec.c v410enc.c vb.c vble.c vc1.c \
		vc1_block.c vc1_loopfilter.c vc1_mc.c vc1_parser.c vc1_pred.c vc1data.c \
		vc1dec.c vc1dsp.c vc2enc.c vc2enc_dwt.c vcr1.c videodsp.c vima.c \
		vmdaudio.c vmdvideo.c vmnc.c vorbis.c vorbis_data.c \
		vorbisdec.c vorbisdsp.c vorbisenc.c vp3.c vp3_parser.c vp3dsp.c vp5.c \
		vp56.c vp56data.c vp56dsp.c vp56rac.c vp6.c vp6dsp.c vp8.c \
		vp8_parser.c vp8dsp.c vp9.c vp9_parser.c \
		vp9block.c \
		vp9data.c vp9dsp.c vp9dsp_10bpp.c vp9dsp_12bpp.c vp9dsp_8bpp.c vp9lpf.c \
		vp9mvs.c vp9prob.c vp9recon.c vqavideo.c \
		wavpack.c \
		wavpackenc.c \
		webp.c \
		webvttdec.c webvttenc.c wma.c wma_common.c wma_freqs.c wmadec.c wmaenc.c \
		wmalosslessdec.c wmaprodec.c wmavoice.c wmv2.c wmv2data.c wmv2dec.c wmv2dsp.c \
		wmv2enc.c wnv1.c wrapped_avframe.c ws-snd1.c \
		
endif

# add for media server
SERVER_SRC += \
		golomb.c \

BSF_SRC += \
		aac_adtstoasc_bsf.c \
		mpeg4audio.c \
		aacadtsdec.c \
		chomp_bsf.c \
		dump_extradata_bsf.c \
		dca_core_bsf.c \
		extract_extradata_bsf.c \
		h2645_parse.c \
		h264_mp4toannexb_bsf.c \
		hevc_mp4toannexb_bsf.c \
		imx_dump_header_bsf.c \
		mjpeg2jpeg_bsf.c \
		jpegtables.c \
		mjpega_dump_header_bsf.c \
		mp3_header_decompress_bsf.c \
		mpegaudiodata.c \
		mpeg4_unpack_bframes_bsf.c \
		movsub_bsf.c \
		noise_bsf.c \
		remove_extradata_bsf.c \
		vp9_superframe_bsf.c \
		$(SERVER_SRC) \



# add exif.c, etc. what are needed by avidec.c
AVI_DEC_SUPPORT_SRC += \
		tiff_common.c \
		exif.c \

SRC += \
		allcodecs.c \
		$(BSF_SRC) \
		aac_ac3_parser.c \
		ac3_parser.c \
		ac3tab.c \
		bitstream.c \
		bitstream_filter.c \
		bitstream_filters.c \
		bsf.c \
		avpacket.c \
		codec_desc.c \
		frame_thread_encoder.c \
		mpegaudiodecheader.c \
		pthread.c \
		pthread_frame.c \
		pthread_slice.c \
		options.c \
		parser.c \
		profiles.c \
		raw.c \
		simple_idct.c \
		utils.c \
		vorbis_parser.c \
		xiph.c \
		$(AVI_DEC_SUPPORT_SRC) \


TARGET_LIB:=libavcodec

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

