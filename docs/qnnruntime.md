*Menu*:
- [1. Introduction](#1-introduction)
- [2. QnnRuntime Data Structures](#2-qnnruntime-data-structures)
- [3. QnnRuntime APIs](#3-qnnruntime-apis)
- [4. QnnRuntime Examples](#4-qnnruntime-examples)
  - [4.1 Set up QnnRuntime configuration](#41-set-up-qnnruntime-configuration)
  - [4.2 Load qnn model from configuration](#42-load-qnn-model-from-configuration)
  - [4.3 Get qnn input/output tensor information](#43-get-qnn-inputoutput-tensor-information)
  - [4.4 Use QnnRuntime::Execute to run qnn inference](#44-use-qnnruntimeexecute-to-run-qnn-inference)
  - [4.5 Register/DeRegister Buffers](#45-registerderegister-buffers)
- [5. References](#5-references)


# 1. Introduction
QnnRuntime is an AI infenrence framework designed to assist users in running QNN models on HTP, GPU, and CPU backends in real-time. It offers a user-friendly approach to facilitate this process:
- load model either from shared library or serialized context binary.
- get detailed input/output tensor information from given models.
- provide qnn performance information during qnn model execution.
- support zero copy mechanism to reduce run-time lantency as much as possible.

# 2. QnnRuntime Data Structures

- [QnnRuntime_Perf_t](../include/ridehal/component/QnnRuntime.hpp#L34)
- [QnnRuntime_UdoPackage_t](../include/ridehal/component/QnnRuntime.hpp#L41)
- [QnnRuntime_LoadType_e](../include/ridehal/component/QnnRuntime.hpp#L49)
- [QnnRuntime_Config_t](../include/ridehal/component/QnnRuntime.hpp#L64)
- [QnnRuntime_TensorInfo_t](../include/ridehal/component/QnnRuntime.hpp#L73)
- [QnnRuntime_TensorInfoList_t](../include/ridehal/component/QnnRuntime.hpp#L80)

# 3. QnnRuntime APIs

- [QnnRuntime::Init](../include/ridehal/component/QnnRuntime.hpp#L103) Initialize QnnRuntime component

- [QnnRuntime::GetInputInfo](../include/ridehal/component/QnnRuntime.hpp#L112) Get input tensor information

- [QnnRuntime::GetOutputInfo](../include/ridehal/component/QnnRuntime.hpp#L120) Get output tensor information

- [QnnRuntime::RegisterBuffers](../include/ridehal/component/QnnRuntime.hpp#L129) Rigister memory with specific shared buffers

- [QnnRuntime::Start](../include/ridehal/component/QnnRuntime.hpp#L137) Start the QnnRuntime object

- [QnnRuntime::EnablePerf](../include/ridehal/component/QnnRuntime.hpp#L144) Enable qnn performance calculation

- [QnnRuntime::Execute](../include/ridehal/component/QnnRuntime.hpp#L155) Execute qnn model with input and output buffer

- [QnnRuntime::GetPerf](../include/ridehal/component/QnnRuntime.hpp#L164) Get qnn latest performance data

- [QnnRuntime:DisablePerf](../include/ridehal/component/QnnRuntime.hpp#L171) Disable qnn performance calculation

- [QnnRuntime::Stop](../include/ridehal/component/QnnRuntime.hpp#L178) Stop the QnnRuntime object

- [QnnRuntime::DeRegisterBuffers](../include/ridehal/component/QnnRuntime.hpp#L187) DeRigister memory with specific shared buffers

- [QnnRuntime::Deinit](../include/ridehal/component/QnnRuntime.hpp#L195) Deinit the QnnRuntime object

# 4. QnnRuntime Examples

## 4.1 Set up QnnRuntime configuration

QnnRuntime configuration help user to set up necessary information before loading qnn model. It mainly includes:
- Specify qnn model file path
- Select qnn model loading type
- Select qnn backend type
- Select qnn priority
- Specify qnn customer op packakes
- Specify qnn model context buffer if loading from binary buffer

Please refer below code block:
```c++
QnnRuntime_Config_t qnnConfig;

// Specify qnn model file path
qnnConfig.modelPath = "data/centernet/program.bin";
// Select qnn model loading type
qnnConfig.loadType = QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE;

// Select qnn backend type
qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;
```

## 4.2 Load qnn model from configuration

Once configuration setup is ready, we can call [QnnRuntime::Init](../include/ridehal/component/QnnRuntime.hpp#L103) and the system will help us automatically load qnn model. Please refer below code block on how to achieve this:

```c++
qnnRuntime.Init( pName, &qnnConfig );
```

## 4.3 Get qnn input/output tensor information

Before we start to create proper input/output buffers, we need to obtain input/output tensor information according to model details. QnnRuntime provides [QnnRuntime::GetInputInfo](../include/ridehal/component/QnnRuntime.hpp#L111) and [QnnRuntime::GetOutputInfo](../include/ridehal/component/QnnRuntime.hpp#L120) to help user get necessnary information. Please refer below code block on how to achieve this:

```c++
// Get input tensor information
QnnRuntime_TensorInfoList_t tensorInputList;
qnnRuntime.GetInputInfo( &tensorInputList );
// Allocate sharedBuffers for input buffer
const uint32_t inputNum = tensorInputList.num;
RideHal_SharedBuffer_t inputs[inputNum];
for ( int i = 0; i < inputNum; ++i )
{
    const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
}

// get output tensor information
QnnRuntime_TensorInfoList_t tensorOutputList;
qnnRuntime.GetOutputInfo( &tensorOutputList );

// Allocate sharedBuffers for output buffer
const uint32_t outputNum = tensorOutputList.num;
RideHal_SharedBuffer_t outputs[outputNum];
for ( int i = 0; i < outputNum; ++i )
{
    const auto ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
}
```


## 4.4 Use QnnRuntime::Execute to run qnn inference

Once user successfully load qnn model and create input/output buffers, then it's time to feed them into qnn context and execute qnn inference cycles. With [QnnRuntime::Execute](../include/ridehal/component/QnnRuntime.hpp#L155), Please refer below code block on how to achieve this:

```c++
// Execute qnn model inference
// variables of inputs, inputNum, outputs, outputNum refer to 4.3
qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
```


## 4.5 Register/DeRegister Buffers

Addtionally, QnnRuntime provides independent interfaces - [QnnRuntime::RegisterBuffers](../include/ridehal/component/QnnRuntime.hpp#L129)/[QnnRuntime::DeRegisterBuffers](../include/ridehal/component/QnnRuntime.hpp#L187) to register/deregister buffers. Actually, it is a bridge mapping buffer address between CPU and HTP. Qnn inference could run on registered buffers without creating a new buffer space for input and output data.
```c++
// Get input tensor information
QnnRuntime_TensorInfoList_t tensorInputList;
qnnRuntime.GetInputInfo( &tensorInputList );
// Allocate sharedBuffers for input buffer
const uint32_t inputNum = tensorInputList.num;
RideHal_SharedBuffer_t inputs[inputNum];
for ( int i = 0; i < inputNum; ++i )
{
    const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
}

// Register input buffers
qnnRuntime.RegisterBuffers(inputs,inputNum );

// DeRgister input buffers
qnnRuntime.DeRegisterBuffers(inputs,inputNum );
```

# 5. References

- [SampleQnn](../tests/sample/source/SampleQnn.cpp#L1).
- [gtest QnnRuntime](../tests/unit_test/components/QnnRuntime/gtest_QnnRuntime.cpp#L1).





