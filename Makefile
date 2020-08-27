CC=aarch64-linux-gnu-gcc
CXX=aarch64-linux-gnu-g++

all:
	make -C libion
	make -C libge2d
	make -C libencoder/h264/bjunion_enc
	make -C libencoder/h264
	make -C libencoder/h265/EncoderAPI-HEVC/hevc_enc
	make -C libencoder/h265/EncoderAPI-HEVC

install: all
	mkdir -p .tmp/usr/lib/aarch64-linux-gnu/ .tmp/usr/local/bin/
	cp libion/*.so .tmp/usr/lib/aarch64-linux-gnu/
	cp libge2d/libge2d/*.so .tmp/usr/lib/aarch64-linux-gnu/
	cp libencoder/h264/bjunion_enc/*.so .tmp/usr/lib/aarch64-linux-gnu/
	cp libencoder/h264/h264EncoderDemo .tmp/usr/local/bin/
	cp libencoder/h265/EncoderAPI-HEVC/hevc_enc/*.so .tmp/usr/lib/aarch64-linux-gnu/
	cp libencoder/h265/EncoderAPI-HEVC/h265EncoderDemo .tmp/usr/local/bin/


clean:
	make -C libion clean
	make -C libge2d clean
	make -C libencoder/h264/bjunion_enc clean
	make -C libencoder/h264 clean
	make -C libencoder/h265/EncoderAPI-HEVC/hevc_enc clean
	make -C libencoder/h265/EncoderAPI-HEVC clean
	rm -rf .tmp
