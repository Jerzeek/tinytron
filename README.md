# ESP32-MiniTV

⚠️ Work in progress ⚠️

This project is a tiny video player designed specifically for the [ESP32-S3-LCD-1.69 from Waveshare](https://www.waveshare.com/wiki/ESP32-S3-LCD-1.69).

It can play MJPEG AVI files from an SD card, or stream video content from a computer over WiFi and Websockets. It features a simple web interface for configuration and can be controlled with a single button.

## Preparing video files

You'll need a FAT32 formatted SD Card, and properly encoded video files (AVI MJPEG). Keep the file names short, and place the files at the root of the SD Card. They will play in alphabetical order.

### Transcoding

You can use [this web page](https://t0mg.github.io/esp32-minitv/transcode.html) to convert video files in the expected format (max. output size 2Gb). It relies on [ffmpeg.wasm](https://github.com/ffmpegwasm/ffmpeg.wasm) for purely local, browser based conversion.

For much faster conversion, the `ffmpeg` command line tool is recommended. 

TODO: add detailed command line

## Credits and references
- This project is relying heavily on [exp32-tv by atomic14](https://github.com/atomic14/esp32-tv) and the related [blog](http://www.atomic14.com) and [videos](https://www.youtube.com/atomic14). Many thanks !
- Another great source was [moononournation's MiniTV](https://github.com/moononournation/MiniTV)
- The web UI uses the VCR OSD Mono font by Riciery Leal
- Github pages hosted [transcoder tool](https://t0mg.github.io/esp32-minitv/transcode.html) inspired by [this post](https://dannadori.medium.com/how-to-deploy-ffmpeg-wasm-application-to-github-pages-76d1ca143b17), uses [coi-serviceworker](https://github.com/gzuidhof/coi-serviceworker) to load [ffmpeg.wasm](https://github.com/ffmpegwasm/ffmpeg.wasm).