# Build

```shell
$ cd multiEnc
$ make -C vpuapi
$ make -C amvenc_lib/
$ make -C amvenc_test/
```

Releated libraries and binary.

```shell
$ ls vpuapi/libamvenc_api.so amvenc_lib/libvpcodec.so amvenc_test/aml_enc_test
amvenc_lib/libvpcodec.so  amvenc_test/aml_enc_test  vpuapi/libamvenc_api.so
```

Update libraries:

```shell
$ sudo cp amvenc_lib/libvpcodec.so /usr/lib/aarch64-linux-gnu
$ sudo cp vpuapi/libamvenc_api.so /usr/lib/aarch64-linux-gnu
```
