
# Class List


Here are the classes, structs, unions and interfaces with brief descriptions:

* **namespace** [**star**](namespacestar.md)     
    * **class** [**AWSConfigParser**](classstar_1_1AWSConfigParser.md) _AWS Configuration file parser._     
    * **class** [**AWSTokenCache**](classstar_1_1AWSTokenCache.md) _AWS SSO Token Cache reader._     
    * **class** [**AWSV4Signer**](classstar_1_1AWSV4Signer.md) _AWS Signature Version 4 signer._     
    * **struct** [**BlockInfo**](structstar_1_1BlockInfo.md) _Metadata for a single compressed block._     
    * **struct** [**BlockMap**](structstar_1_1BlockMap.md) _Maps logical elements to physical blocks._     
    * **struct** [**ByteUnshuffleSpec**](structstar_1_1ByteUnshuffleSpec.md) _Describes an in-place byte-unshuffle to fuse into an array's fill._     
    * **struct** [**ColdStorage**](structstar_1_1ColdStorage.md) _Cold storage - infrequently accessed data._     
    * **struct** [**ExtractionPlan**](structstar_1_1ExtractionPlan.md) _Describes how to extract elements from blocks._     
        * **struct** [**ElementRange**](structstar_1_1ExtractionPlan_1_1ElementRange.md)     
    * **struct** [**FileHeader**](structstar_1_1FileHeader.md) _File header structure (31 bytes fixed size)._     
    * **struct** [**FilePathInfo**](structstar_1_1FilePathInfo.md)     
    * **struct** [**HotStorage**](structstar_1_1HotStorage.md) _Hot storage - frequently accessed data (cache-friendly)._     
    * **class** [**HttpRangeReader**](classstar_1_1HttpRangeReader.md)     
    * **struct** [**IndexEntry**](structstar_1_1IndexEntry.md) _Index entry with block compression support and shape information._     
    * **struct** [**KeyRegistry**](structstar_1_1KeyRegistry.md) _Global key registry using data-oriented design (Structure of Arrays)._     
    * **class** [**LayerMetadataAccessor**](classstar_1_1LayerMetadataAccessor.md) _Metadata accessor for a specific layer with inheritance._     
    * **struct** [**LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md) _Per-layer metadata registry using data-oriented design (Structure of Arrays)._     
    * **class** [**LayerView**](classstar_1_1LayerView.md) _Lightweight view into a specific layer with inheritance from base._     
    * **class** [**LocalRangeReader**](classstar_1_1LocalRangeReader.md)     
    * **class** [**MemoryRangeReader**](classstar_1_1MemoryRangeReader.md)     
    * **class** [**MetadataAccessor**](classstar_1_1MetadataAccessor.md) _Accessor for metadata operations._     
    * **struct** [**MetadataValue**](structstar_1_1MetadataValue.md) _Type-erased wrapper for metadata values._     
    * **class** [**NDArray**](classstar_1_1NDArray.md) _Modern n-dimensional array class with xtensor-style API._     
    * **struct** [**OpenOptions**](structstar_1_1OpenOptions.md) _Read-time options for_ [_**StarDataset::open()**_](classstar_1_1StarDataset.md#function-open-12) _._    
    * **class** [**RangeReader**](classstar_1_1RangeReader.md)     
    * **struct** [**S3Credentials**](structstar_1_1S3Credentials.md) _AWS Credentials with resolution chain._     
    * **struct** [**S3EndpointConfig**](structstar_1_1S3EndpointConfig.md) _S3 endpoint resolution (default AWS, or an override for S3-compatible services such as MinIO and for local testing)._     
    * **class** [**S3RangeReader**](classstar_1_1S3RangeReader.md)     
    * **class** [**S3Writer**](classstar_1_1S3Writer.md) _S3 writer for uploading objects._     
        * **struct** [**HeaderData**](structstar_1_1S3Writer_1_1HeaderData.md)     
    * **struct** [**Slice**](structstar_1_1Slice.md) _Describes a slice along one dimension (Python-style slicing) Plain struct - no methods except helpers, just data._     
    * **struct** [**SliceSpec**](structstar_1_1SliceSpec.md) _Complete slice specification for n-dimensional array._     
    * **struct** [**StarConfig**](structstar_1_1StarConfig.md) _Configuration for metadata block optimization._     
    * **class** [**StarDataset**](classstar_1_1StarDataset.md) _A cloud-optimized binary key-value store for serializable data types._     
    * **class** [**ThreadPool**](classstar_1_1ThreadPool.md) _Simple thread pool for parallel block operations._     
    * **struct** [**TypeToDataType**](structstar_1_1TypeToDataType.md) 
    * **struct** [**TypeToDataType&lt; double &gt;**](structstar_1_1TypeToDataType_3_01double_01_4.md)     
    * **struct** [**TypeToDataType&lt; float &gt;**](structstar_1_1TypeToDataType_3_01float_01_4.md)     
    * **struct** [**TypeToDataType&lt; int16\_t &gt;**](structstar_1_1TypeToDataType_3_01int16__t_01_4.md)     
    * **struct** [**TypeToDataType&lt; int32\_t &gt;**](structstar_1_1TypeToDataType_3_01int32__t_01_4.md)     
    * **struct** [**TypeToDataType&lt; int64\_t &gt;**](structstar_1_1TypeToDataType_3_01int64__t_01_4.md)     
    * **struct** [**TypeToDataType&lt; int8\_t &gt;**](structstar_1_1TypeToDataType_3_01int8__t_01_4.md)     
    * **struct** [**TypeToDataType&lt; std::string &gt;**](structstar_1_1TypeToDataType_3_01std_1_1string_01_4.md)     
    * **struct** [**TypeToDataType&lt; uint16\_t &gt;**](structstar_1_1TypeToDataType_3_01uint16__t_01_4.md)     
    * **struct** [**TypeToDataType&lt; uint32\_t &gt;**](structstar_1_1TypeToDataType_3_01uint32__t_01_4.md)     
    * **struct** [**TypeToDataType&lt; uint64\_t &gt;**](structstar_1_1TypeToDataType_3_01uint64__t_01_4.md)     
    * **struct** [**TypeToDataType&lt; uint8\_t &gt;**](structstar_1_1TypeToDataType_3_01uint8__t_01_4.md)     
    * **namespace** [**logger**](namespacestar_1_1logger.md)     
    * **namespace** [**s3crypto**](namespacestar_1_1s3crypto.md)     
    * **namespace** [**shuffle\_detail**](namespacestar_1_1shuffle__detail.md)     
* **struct** [**SSOToken**](structstar_1_1AWSTokenCache_1_1SSOToken.md)     
* **namespace** [**std**](namespacestd.md) 

