#
# $Id$
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = . x86

C_FLAGS +=  $(FFMPEG_CFLAGS) -Iarm

################################################
####   source file 
################################################
ARCH_SRC += \

# add AAC demuxer, Nov.1st,2017
DECODER_SRC += \
		asf.c \
		asfcrypt.c \
		asfdec_f.c \
		avidec.c \
		flvdec.c \
		m4vdec.c \
		matroskadec.c \
		mp3dec.c \
		riff.c \
		riffdec.c \
		aacdec.c \
		img2.c \
		img2_alias_pix.c \
		img2_brender_pix.c \
		img2dec.c \
		gifdec.c \
		mpegvideodec.c \
		mpeg.c \


ENCODER_SRC += \
		riffenc.c \
		avienc.c \
		matroskaenc.c \
		avc.c \
		hevc.c \
		flacenc.c \
		flacenc_header.c \
		flac_picture.c \
		movenc.c \
		movenccenc.c \
		movenchint.c \
		vpcc.c \
		mp3enc.c \
		oggparsevorbis.c \
		rawutils.c \
		replaygain.c \
		rtp.c \
		rtpenc_chain.c \
		qtpalette.c \
		vorbiscomment.c \
		wv.c \



URL_SRC += \
		file.c \
		ftp.c \
		protocols.c \



MPEG_TS_SRC += \
		mpegts.c \
		rm.c \
		rmdec.c \
		

# rtmpProtocol also dependent on libavutil/hmac.c
# add for media server
SERVER_SRC += \
		flvenc.c \
		hlsenc.c \
		http.c \
		httpauth.c \
		rtmpproto.c \
		rtmppkt.c \
		rtpdec.c \
		rtpproto.c \
		udp.c \
		rtpdec_ac3.c rtpdec_amr.c rtpdec_asf.c \
		rtpdec_dv.c rtpdec_g726.c rtpdec_h261.c rtpdec_h263.c rtpdec_h263_rfc2190.c \
		rtpdec_h264.c rtpdec_hevc.c rtpdec_ilbc.c rtpdec_jpeg.c rtpdec_latm.c \
		rtpdec_mpa_robust.c rtpdec_mpeg12.c rtpdec_mpeg4.c rtpdec_mpegts.c rtpdec_qcelp.c \
		rtpdec_qdm2.c rtpdec_qt.c rtpdec_svq3.c rtpdec_vc2hq.c rtpdec_vp8.c \
		rtpdec_vp9.c rtpdec_xiph.c rtpenc.c rtpenc_aac.c rtpenc_amr.c \
		rtpenc_h261.c rtpenc_h263.c rtpenc_h263_rfc2190.c rtpenc_h264_hevc.c rtpenc_jpeg.c \
		rtpenc_latm.c rtpenc_mpegts.c rtpenc_mpv.c rtpenc_vc2hq.c rtpenc_vp8.c rtpenc_vp9.c \
		rtpenc_xiph.c rtspenc.c \
		rtspdec.c \
		rtsp.c \
		rdt.c \
		srtp.c \
		latmenc.c \
		adtsenc.c \
		apetag.c \
		tcp.c \
		
# Oct.13,2017 add latmenc.c and adtsenc.c apetag.c for AAC transfer format
# Oct.27,2017 add tcp.c for rtmp protocol

SRC += \
		avio.c \
		aviobuf.c \
		avlanguage.c \
		cutils.c \
		matroska.c \
		mov.c \
		mov_chan.c \
		dump.c \
		format.c \
		id3v1.c \
		id3v2.c \
		id3v2enc.c \
		isom.c \
		metadata.c \
		mpegtsenc.c \
		mux.c \
		network.c \
		options.c \
		os_support.c \
		rawdec.c \
		rawenc.c \
		rmsipr.c \
		unix.c \
		url.c \
		urldecode.c \
		utils.c \
		utils.c \
		sdp.c \
		allformats.c \
		$(DECODER_SRC) \
		$(ENCODER_SRC) \
		$(URL_SRC) \
		$(MPEG_TS_SRC) \
		$(SERVER_SRC) \




ifeq ($(ARCH),arm)
else
SRC += \
		$(ARCH_SRC) \
		3dostr.c 4xm.c a64.c \
		aadec.c \
		ac3dec.c \
		acm.c act.c adp.c ads.c \
		adxdec.c \
		aea.c afc.c aiffdec.c aiffenc.c aixdec.c  \
		amr.c anm.c apc.c ape.c apngdec.c \
		apngenc.c \
		aqtitledec.c \
		asfdec_o.c \
		asfenc.c \
		assdec.c \
		assenc.c \
		ast.c \
		astdec.c \
		astenc.c \
		async.c au.c \
		audiointerleave.c \
		avr.c avs.c \
		bethsoftvid.c \
		bfi.c bink.c bintext.c bit.c bmv.c \
		boadec.c brstm.c \
		c93.c \
		cache.c caf.c cafdec.c \
		cafenc.c cavsvideodec.c cdg.c cdxl.c cinedec.c concat.c \
		concatdec.c crcenc.c crypto.c \
		dashenc.c data_uri.c \
		dauddec.c daudenc.c dcstr.c dfa.c diracdec.c dnxhddec.c \
		dsfdec.c dsicin.c dss.c dtsdec.c dtshddec.c \
		dv.c dvbsub.c dvbtxt.c dvenc.c dxa.c \
		eacdata.c \
		electronicarts.c epafdec.c ffmdec.c ffmenc.c ffmetadec.c \
		ffmetaenc.c \
		fifo.c \
		filmstripdec.c \
		filmstripenc.c \
		flacdec.c \
		flic.c \
		framecrcenc.c framehash.c \
		frmdec.c \
		fsb.c \
		g722.c g723_1.c g729dec.c \
		genh.c gif.c gopher.c gsmdec.c gxf.c \
		gxfenc.c \
		h261dec.c h263dec.c h264dec.c \
		hashenc.c hdsenc.c \
		hevcdec.c hls.c hlsproto.c hnm.c \
		icecast.c icodec.c icoenc.c \
		idcin.c idroqdec.c idroqenc.c iff.c \
		ilbc.c \
		img2enc.c ingenientdec.c ipmovie.c ircam.c ircamdec.c ircamenc.c \
		iss.c iv8.c ivfdec.c ivfenc.c \
		jacosubdec.c jacosubenc.c \
		jvdec.c \
		lmlm4.c loasdec.c lrc.c lrcdec.c lrcenc.c \
		lvfdec.c lxfdec.c \
		md5proto.c \
		mgsts.c \
		microdvddec.c microdvdenc.c mj2kdec.c \
		mkvtimestamp_v2.c mlpdec.c mlvdec.c mm.c mmf.c mms.c mmsh.c \
		mmst.c \
		mpc.c mpc8.c \
		mpegenc.c \
		mpjpeg.c mpjpegdec.c mpl2dec.c mpsubdec.c msf.c \
		msnwc_tcp.c mtaf.c mtv.c musx.c \
		mvdec.c mvi.c \
		mxf.c mxfdec.c mxfenc.c mxg.c \
		ncdec.c \
		nistspheredec.c \
		nsvdec.c nullenc.c nut.c nutdec.c nutenc.c nuv.c \
		oggdec.c \
		oggenc.c oggparsecelt.c oggparsedaala.c oggparsedirac.c oggparseflac.c \
		oggparseogm.c oggparseopus.c oggparseskeleton.c \
		oggparsespeex.c oggparsetheora.c \
		oggparsevp8.c oma.c omadec.c omaenc.c \
		paf.c pcm.c pcmdec.c pcmenc.c pjsdec.c \
		pmpdec.c prompeg.c \
		psxstr.c pva.c pvfdec.c \
		qcp.c \
		r3d.c \
		rawvideodec.c \
		realtextdec.c redspark.c \
		rl2.c rmenc.c \
		rpl.c rsd.c rso.c rsodec.c rsoenc.c rtmphttp.c \
		samidec.c \
		sapdec.c \
		sapenc.c \
		sauce.c sbgdec.c sccdec.c sccenc.c \
		sdr2.c sdsdec.c \
		sdxdec.c segafilm.c segment.c shortendec.c sierravmd.c siff.c smacker.c \
		smjpeg.c smjpegdec.c smjpegenc.c smoothstreamingenc.c smush.c sol.c soxdec.c \
		soxenc.c spdif.c spdifdec.c spdifenc.c srtdec.c srtenc.c \
		srtpproto.c stldec.c subfile.c subtitles.c subviewer1dec.c subviewerdec.c \
		supdec.c svag.c swf.c swfdec.c swfenc.c \
		takdec.c \
		tedcaptionsdec.c \
		tee.c tee_common.c teeproto.c thp.c tiertexseq.c \
		tmv.c \
		tta.c \
		ttaenc.c \
		tty.c \
		txd.c \
		uncodedframecrcenc.c \
		v210.c vag.c vc1dec.c \
		vc1test.c vc1testenc.c vivo.c \
		voc.c voc_packet.c vocdec.c vocenc.c \
		vpk.c vplayerdec.c vqf.c \
		w64.c \
		wavdec.c \
		wavenc.c \
		wc3movie.c webm_chunk.c webmdashenc.c webpenc.c webvttdec.c \
		webvttenc.c westwood_aud.c westwood_vqa.c wsddec.c wtv_common.c wtvdec.c \
		wtvenc.c \
		wvdec.c wvedec.c wvenc.c \
		xa.c xmv.c xvag.c \
		xwma.c \
		yop.c yuv4mpegdec.c yuv4mpegenc.c
		
endif



TARGET_LIB:=libavformat

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

