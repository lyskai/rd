
# Tools that convert video files to the inputs of the Sample DataReader.


## Usage

Please note to generate images with width and height is aligned according to the hardware zero copy requirement, generally, for the width, it should be 128 aligned, for the height, it should be 32 aligned.

```sh
usage: video2dr.py [-h] -i INPUT [-o OUTPUT] [-s OFFSET] [-m MAXIMUM]
                   [-r RESOLUTION] [-f FORMAT]

convert video to inputs of the Sample DataReader

optional arguments:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        The input video file(*.mp4)
  -o OUTPUT, --output OUTPUT
                        The output path to store the generated inputs
  -s OFFSET, --offset OFFSET
                        the offset of the frame in the video
  -m MAXIMUM, --maximum MAXIMUM
                        The maximum number of the generated inputs
  -r RESOLUTION, --resolution RESOLUTION
                        The resolution of camera
  -f FORMAT, --format FORMAT
                        The color format of camera, [uyvy, nv12, nv21, rgb]
```

## examples commands

```sh
python scripts/utils/data_reader/video2dr.py -i 4K_Street.mp4 -s 1000 -m 50 -r "3840 2176" -f nv12 -o 4K_street_1000_50_3840_2176_nv12
python scripts/utils/data_reader/video2dr.py -i 4K_Street.mp4 -s 1000 -m 50 -r "1920 1024" -f uyvy -o 4K_street_1000_50_1920_1024_uyvy
```