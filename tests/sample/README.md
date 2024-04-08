# RideHal Sample Application

This RideHal sample is an application to demonstrate how to use the RideHal components.

## 1. RideHal Sample Application command line arguments

Below is a sample that how to using command line arguments to tell the RideHal sample application to create the component with the specified arguments.

```sh
export RIDEHAL_LOG_LEVEL=INFO
./bin/rhrun ./bin/RideHalSampleApp \
  -n component0_name -t component0_type \
    -k component0_attr1_name -v component0_attr1_value \
    -k component0_attr2_name -v component0_attr2_value \
    ... \
  -n componentX_name -t componentX_type \
    -k componentX_attr1_name -v componentX_attr1_value \
    -k componentX_attr2_name -v componentX_attr3_value \
```

Note: the "-n componentX_name -t componentX_type" must be in the begin for each component, and all the repeated "-k componentX_attr?_name -v componentX_attr?_value" are going to specify the attributes for this component.

| parameter | required | type      | comments |
|-----------|----------|-----------|----------|
| -n        | true     | string    | The unique component name |
| -t        | true     | string    | The component type name, options from [DataReader, Camera, Remap, Qnn, C2D, PostProcCenternet, TinyViz] |
| -k        | true     | string    | The unique component attribute name |
| -v        | true     | string    | The attribute value for the prvious attribute name |

## 2. RideHal Samples

### 2.1 RideHal DataReader Sample

| attribute | required | type      | default | comments |
|-----------|----------|-----------|---------|----------|
| format    | false    | string    | "nv12"  | The image format, options from [nv12, uyvy, rgb, bgr, p010] |
| width     | false    | int       | 1920    | The image width |
| height    | false    | int       | 1024    | The image height |
| fps       | false    | int       | 30      | The frame rate per second |
| data_path | true     | string    | -       | The data path that contain the image files |
| pool_size | false    | int       | 4       | the image memory pool size |
| topic     | true     | string    | -       | the output topic name |

The command line template example:

```sh
  -n CAM1 -t DataReader -k width -v 1920 -k height -v 1024 \
    -k data_path -v /data/4K_street_1000_500_1920_1024_uyvy \
    -k pool_size -v 4 -k format -v uyvy \
    -k topic -v /sensor/camera/CAM1/raw \
```

### 2.2 RideHal Camera Sample

| attribute | required | type      | default | comments |
|-----------|----------|-----------|---------|----------|
| input_id  | true     | int       | -       | The camera input id |
| width     | true     | int       | -       | The image width |
| height    | true     | int       | -       | The image height |
| topic     | true     | string    | -       | the output topic name |

The command line template example:

```sh
  -n CAM0 -t Camera -k input_id -v 0 \
    -k width -v 1928 -k height -v 1208 \
    -k topic -v /sensor/camera/CAM0/raw \
```

### 2.3 RideHal C2D Sample

| attribute     | required | type      | default | comments |
|---------------|----------|-----------|---------|----------|
| output_width  | false    | int       | 1152    | The output image width |
| output_height | false    | int       | 800     | The output image height |
| output_format | false    | string    | rgb     | The output image format, options from [rgb]|
| batch_size    | false    | int       | 1       | The Remap input batch size |
| input_widthX  | false    | int       | 1920    | The input X image width |
| input_heightX | false    | int       | 1024    | The input X image height |
| input_formatX | false    | string    | uyvy    | The input X image format, options from [uyvy]|
| roi_xX        | false    | int       | 0       | The ROI top x for input X |
| roi_yX        | false    | int       | 0       | The ROI top y for input X |
| roi_widthX    | false    | int       | =output_width  | The ROI width for input X |
| roi_heightX   | false    | int       | =output_height | The ROI height for input X |
| pool_size     | false    | int       | 4       | the image memory pool size |
| input_topic   | true     | string    | -       | the input topic name |
| output_topic  | true     | string    | -       | the output topic name |

Note: "X" is value from 0 to batch_size-1, thus the attribute with suffix "X" is repeated for different input batch.

The command line template example:

```sh
  -n C2D0 -t C2D -k batch_size -v 1 \
    -k input_width0 -v 2048 -k input_height0 -v 1216 -k input_format0 -v nv12 \
    -k roi_x0 -v 0 -k roi_y0 -v 0 -k roi_width0 -v 1928 -k roi_height0 -v 1208 \
    -k output_width -v 2048 -k output_height -v 1216 -k output_format -v uyvy \
    -k input_topic -v /sensor/camera/CAM0/raw \
    -k output_topic -v /sensor/camera/CAM0/uyvy \
```

### 2.4 RideHal Remap Sample

| attribute     | required | type      | default | comments |
|---------------|----------|-----------|---------|----------|
| processor     | false    | string    | "dsp0"  | The processor type, options from [dsp0, dsp1, cpu, gpu] |
| output_width  | false    | int       | 1152    | The output image width |
| output_height | false    | int       | 800     | The output image height |
| output_format | false    | string    | rgb     | The output image format, options from [rgb]|
| batch_size    | false    | int       | 1       | The Remap input batch size |
| input_widthX  | false    | int       | 1920    | The input X image width |
| input_heightX | false    | int       | 1024    | The input X image height |
| input_formatX | false    | string    | uyvy    | The input X image format, options from [uyvy]|
| map_widthX    | false    | int       | =output_width  | The map width for input X |
| map_heightX   | false    | int       | =output_height | The map height for input X |
| roi_xX        | false    | int       | 0       | The ROI top x for input X |
| roi_yX        | false    | int       | 0       | The ROI top y for input X |
| roi_widthX    | false    | int       | =output_width  | The ROI width for input X |
| roi_heightX   | false    | int       | =output_height | The ROI height for input X |
| pool_size     | false    | int       | 4       | the image memory pool size |
| quant_scale   | false    | float     | 0.0186584480106831 | The quantization scale of the quantize model input  |
| quant_offset  | false    | int       | 114     | The quantization offset of the quantize model input |
| input_topic   | true     | string    | -       | the input topic name |
| output_topic  | true     | string    | -       | the output topic name |

Note: "X" is value from 0 to batch_size-1, thus the attribute with suffix "X" is repeated for different input batch.

The command line template example:

```sh
  -n REMAP0 -t Remap -k batch_size -v 1 \
    -k input_width0 -v 2048 -k input_height0 -v 1216 -k input_format0 -v uyvy \
    -k output_width -v 1152 -k output_height -v 800 -k output_format -v rgb \
    -k input_topic -v /sensor/camera/CAM0/uyvy \
    -k output_topic -v /sensor/camera/CAM0/remap \
```

### 2.5 RideHal Qnn Sample

| attribute     | required | type      | default | comments |
|---------------|----------|-----------|---------|----------|
| core          | false    | int       | 0       | The HTP core id: 0 - CDSP0, 1 - CDSP1 |
| model_path    | true     | string    | -       | The model path that contain the QNN models |
| pool_size     | false    | int       | 4       | the image memory pool size |
| input_topic   | true     | string    | -       | the input topic name |
| output_topic  | true     | string    | -       | the output topic name |

The command line template example:

```sh
  -n CNT0 -t Qnn -k core -v 0 \
    -k model_path -v /data/pkg-aarch64-qos222/opt/ridehal/data/centernet \
    -k input_topic -v /sensor/camera/CAM0/remap \
    -k output_topic -v /sensor/camera/CAM0/qnn \
```

### 2.6 RideHal PostProcCenternet Sample

| attribute     | required | type      | default | comments |
|---------------|----------|-----------|---------|----------|
| roi_x         | false    | int       | 0       | The ROI top x |
| roi_y         | false    | int       | 0       | The ROI top y |
| width         | false    | int       | 1920    | The ROI width |
| height        | false    | int       | 1024    | The ROI height |
| score_threshold | false  | float     | 0.6     | The score threshold |
| nms_threshold   | false  | float     | 0.6     | The NMS threshold |
| pool_size     | false    | int       | 4       | the image memory pool size |
| input_topic   | true     | string    | -       | the input topic name |
| output_topic  | true     | string    | -       | the output topic name |

The command line template example:

```sh
  -n POSTPROC_CNT0 -t PostProcCenternet \
    -k width -v 1928 -k height -v 1028 \
    -k input_topic -v /sensor/camera/CAM0/qnn \
    -k output_topic -v /sensor/camera/CAM0/objs \
```

### 2.7 RideHal TinyViz Sample

| attribute     | required | type        | default | comments |
|---------------|----------|-------------|---------|----------|
| winW          | false    | int         | 1920    | The window width |
| winH          | false    | int         | 1080    | The window height |
| cameras       | true     | string list | -       | The cameras' name list |
| cam_topicX    | true     | string    | /sensor/camera/${cameras[X]}/raw   | the input camera frame topic name for camera X |
| obj_topicX    | true     | string    | /sensor/camera/${cameras[X]}/objs  | the input road object topic name for camera X |

The command line template example:

```sh
  -n VIZ -t TinyViz -k cameras -v CAM0,CAM1,CAM2,CAM3
```

### 2.8 RideHal VideoEncoder Sample

| attribute     | required | type      | default | comments |
|---------------|----------|-----------|---------|----------|
| width         | true     | int       | -       | The image width |
| height        | true     | int       | -       | The image height |
| pool_size     | false    | int       | 4       | The image memory pool size |
| bitrate       | false    | int       | 8000000 | The encoding bitrate |
| fps           | false    | int       | 30      | The frame rate per second |
| input_topic   | true     | string    | -       | the input topic name |
| output_topic  | true     | string    | -       | the output topic name |

The command line template example:

```sh
  -n VENC0 -t VideoEncoder -k width -v 1920 -k height -v 1024 \
    -k bitrate -v 8000000 \
    -k input_topic -v /sensor/camera/CAM0/raw \
    -k output_topic -v /sensor/camera/CAM0/hevc \
```

### 2.9 RideHal Recorder Sample

| attribute     | required | type      | default | comments |
|---------------|----------|-----------|---------|----------|
| max           | false    | int       | 1000    | The maximum recorded images |
| topic         | true     | string    | -       | the input topic name |

The command line template example:

```sh
  -n REC0 -t Recorder -k max -v 100 -k topic -v /sensor/camera/CAM0/hevc \
```


## 3. Typical RideHal Sample Applicaiton pipelines

### 3.1 4 DataReader based QNN perception pipelines

```sh
export RIDEHAL_LOG_LEVEL=INFO
./bin/rhrun ./bin/RideHalSampleApp -n CAM0 -t DataReader -k width -v 1920 -k height -v 1024 \
    -k data_path -v /data/4K_street_1000_500_1920_1024_uyvy \
    -k format -v uyvy -k topic -v /sensor/camera/CAM0/raw \
  -n REMAP0 -t Remap -k batch_size -v 1 \
    -k input_width0 -v 1920 -k input_height0 -v 1024 -k input_format0 -v uyvy \
    -k output_width -v 1152 -k output_height -v 800 -k output_format -v rgb \
    -k input_topic -v /sensor/camera/CAM0/raw \
    -k output_topic -v /sensor/camera/CAM0/remap \
  -n CNT0 -t Qnn -k core -v 0 \
    -k model_path -v /data/pkg-aarch64-qos222/opt/ridehal/data/centernet \
    -k input_topic -v /sensor/camera/CAM0/remap \
    -k output_topic -v /sensor/camera/CAM0/qnn \
  -n POSTPROC_CNT0 -t PostProcCenternet \
    -k width -v 1920 -k height -v 1024 \
    -k input_topic -v /sensor/camera/CAM0/qnn \
    -k output_topic -v /sensor/camera/CAM0/objs \
  -n CAM1 -t DataReader -k width -v 1920 -k height -v 1024 \
    -k data_path -v /data/4K_street_1000_500_1920_1024_uyvy \
    -k format -v uyvy -k topic -v /sensor/camera/CAM1/raw \
  -n REMAP1 -t Remap -k batch_size -v 1 \
    -k input_width0 -v 1920 -k input_height0 -v 1024 -k input_format0 -v uyvy \
    -k output_width -v 1152 -k output_height -v 800 -k output_format -v rgb \
    -k input_topic -v /sensor/camera/CAM1/raw \
    -k output_topic -v /sensor/camera/CAM1/remap \
  -n CNT1 -t Qnn -k core -v 0 \
    -k model_path -v /data/pkg-aarch64-qos222/opt/ridehal/data/centernet \
    -k input_topic -v /sensor/camera/CAM1/remap \
    -k output_topic -v /sensor/camera/CAM1/qnn \
  -n POSTPROC_CNT1 -t PostProcCenternet \
    -k width -v 1920 -k height -v 1024 \
    -k input_topic -v /sensor/camera/CAM1/qnn \
    -k output_topic -v /sensor/camera/CAM1/objs \
  -n CAM2 -t DataReader -k width -v 1920 -k height -v 1024 \
    -k data_path -v /data/4K_street_1000_500_1920_1024_uyvy \
    -k format -v uyvy -k topic -v /sensor/camera/CAM2/raw \
  -n REMAP2 -t Remap -k batch_size -v 1 \
    -k input_width0 -v 1920 -k input_height0 -v 1024 -k input_format0 -v uyvy \
    -k output_width -v 1152 -k output_height -v 800 -k output_format -v rgb \
    -k input_topic -v /sensor/camera/CAM2/raw \
    -k output_topic -v /sensor/camera/CAM2/remap \
  -n CNT2 -t Qnn -k core -v 0 \
    -k model_path -v /data/pkg-aarch64-qos222/opt/ridehal/data/centernet \
    -k input_topic -v /sensor/camera/CAM2/remap \
    -k output_topic -v /sensor/camera/CAM2/qnn \
  -n POSTPROC_CNT2 -t PostProcCenternet \
    -k width -v 1920 -k height -v 1024 \
    -k input_topic -v /sensor/camera/CAM2/qnn \
    -k output_topic -v /sensor/camera/CAM2/objs \
  -n CAM3 -t DataReader -k width -v 1920 -k height -v 1024 \
    -k data_path -v /data/4K_street_1000_500_1920_1024_uyvy \
    -k format -v uyvy -k topic -v /sensor/camera/CAM3/raw \
  -n REMAP3 -t Remap -k batch_size -v 1 \
    -k input_width0 -v 1920 -k input_height0 -v 1024 -k input_format0 -v uyvy \
    -k output_width -v 1152 -k output_height -v 800 -k output_format -v rgb \
    -k input_topic -v /sensor/camera/CAM3/raw \
    -k output_topic -v /sensor/camera/CAM3/remap \
  -n CNT3 -t Qnn -k core -v 0 \
    -k model_path -v /data/pkg-aarch64-qos222/opt/ridehal/data/centernet \
    -k input_topic -v /sensor/camera/CAM3/remap \
    -k output_topic -v /sensor/camera/CAM3/qnn \
  -n POSTPROC_CNT3 -t PostProcCenternet \
    -k width -v 1920 -k height -v 1024 \
    -k input_topic -v /sensor/camera/CAM3/qnn \
    -k output_topic -v /sensor/camera/CAM3/objs \
  -n VIZ -t TinyViz -k cameras -v CAM0,CAM1,CAM2,CAM3
```

### 3.2 1 DataReader and 1 Camera AR231 based QNN perception pipelines

```sh
export RIDEHAL_LOG_LEVEL=INFO
./bin/rhrun ./bin/RideHalSampleApp -n CAM0 -t Camera -k input_id -v 0 \
    -k width -v 1928 -k height -v 1208 \
    -k topic -v /sensor/camera/CAM0/raw \
  -n C2D0 -t C2D -k batch_size -v 1 \
    -k input_width0 -v 2048 -k input_height0 -v 1216 -k input_format0 -v nv12 \
    -k roi_x0 -v 0 -k roi_y0 -v 0 -k roi_width0 -v 1928 -k roi_height0 -v 1208 \
    -k output_width -v 2048 -k output_height -v 1216 -k output_format -v uyvy \
    -k input_topic -v /sensor/camera/CAM0/raw \
    -k output_topic -v /sensor/camera/CAM0/uyvy \
  -n REMAP0 -t Remap -k batch_size -v 1 \
    -k input_width0 -v 2048 -k input_height0 -v 1216 -k input_format0 -v uyvy \
    -k output_width -v 1152 -k output_height -v 800 -k output_format -v rgb \
    -k input_topic -v /sensor/camera/CAM0/uyvy \
    -k output_topic -v /sensor/camera/CAM0/remap \
  -n CNT0 -t Qnn -k core -v 0 \
    -k model_path -v /data/pkg-aarch64-qos222/opt/ridehal/data/centernet \
    -k input_topic -v /sensor/camera/CAM0/remap \
    -k output_topic -v /sensor/camera/CAM0/qnn \
  -n POSTPROC_CNT0 -t PostProcCenternet \
    -k width -v 1928 -k height -v 1208 \
    -k input_topic -v /sensor/camera/CAM0/qnn \
    -k output_topic -v /sensor/camera/CAM0/objs \
  -n CAM1 -t DataReader -k width -v 1920 -k height -v 1024 \
    -k data_path -v /data/4K_street_1000_500_1920_1024_uyvy \
    -k format -v uyvy -k topic -v /sensor/camera/CAM1/raw \
  -n REMAP1 -t Remap -k batch_size -v 1 \
    -k input_width0 -v 1920 -k input_height0 -v 1024 -k input_format0 -v uyvy \
    -k output_width -v 1152 -k output_height -v 800 -k output_format -v rgb \
    -k input_topic -v /sensor/camera/CAM1/raw \
    -k output_topic -v /sensor/camera/CAM1/remap \
  -n CNT1 -t Qnn -k core -v 0 \
    -k model_path -v /data/pkg-aarch64-qos222/opt/ridehal/data/centernet \
    -k input_topic -v /sensor/camera/CAM1/remap \
    -k output_topic -v /sensor/camera/CAM1/qnn \
  -n POSTPROC_CNT1 -t PostProcCenternet \
    -k width -v 1920 -k height -v 1024 \
    -k input_topic -v /sensor/camera/CAM1/qnn \
    -k output_topic -v /sensor/camera/CAM1/objs \
  -n VIZ -t TinyViz -k cameras -v CAM0,CAM1
```
