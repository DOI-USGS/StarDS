
# Class Hierarchy

This inheritance list is sorted roughly, but not completely, alphabetically:


* **class** [**star::AWSConfigParser**](classstar_1_1AWSConfigParser.md) _AWS Configuration file parser._ 
* **class** [**star::AWSTokenCache**](classstar_1_1AWSTokenCache.md) _AWS SSO Token Cache reader._ 
* **class** [**star::AWSV4Signer**](classstar_1_1AWSV4Signer.md) _AWS Signature Version 4 signer._ 
* **class** [**star::RangeReader**](classstar_1_1RangeReader.md)     
    * **class** [**star::HttpRangeReader**](classstar_1_1HttpRangeReader.md) 
    * **class** [**star::LocalRangeReader**](classstar_1_1LocalRangeReader.md) 
    * **class** [**star::MemoryRangeReader**](classstar_1_1MemoryRangeReader.md) 
    * **class** [**star::S3RangeReader**](classstar_1_1S3RangeReader.md) 
* **class** [**star::LayerMetadataAccessor**](classstar_1_1LayerMetadataAccessor.md) _Metadata accessor for a specific layer with inheritance._ 
* **class** [**star::LayerView**](classstar_1_1LayerView.md) _Lightweight view into a specific layer with inheritance from base._ 
* **class** [**star::MetadataAccessor**](classstar_1_1MetadataAccessor.md) _Accessor for metadata operations._ 
* **class** [**star::NDArray**](classstar_1_1NDArray.md) _Modern n-dimensional array class with xtensor-style API._ 
* **class** [**star::S3Writer**](classstar_1_1S3Writer.md) _S3 writer for uploading objects._ 
* **class** [**star::ThreadPool**](classstar_1_1ThreadPool.md) _Simple thread pool for parallel block operations._ 
* **struct** [**star::BlockInfo**](structstar_1_1BlockInfo.md) _Metadata for a single compressed block._ 
* **struct** [**star::BlockMap**](structstar_1_1BlockMap.md) _Maps logical elements to physical blocks._ 
* **struct** [**star::ByteUnshuffleSpec**](structstar_1_1ByteUnshuffleSpec.md) _Describes an in-place byte-unshuffle to fuse into an array's fill._ 
* **struct** [**star::ColdStorage**](structstar_1_1ColdStorage.md) _Cold storage - infrequently accessed data._ 
* **struct** [**star::ExtractionPlan**](structstar_1_1ExtractionPlan.md) _Describes how to extract elements from blocks._ 
* **struct** [**star::ExtractionPlan::ElementRange**](structstar_1_1ExtractionPlan_1_1ElementRange.md) 
* **struct** [**star::FileHeader**](structstar_1_1FileHeader.md) _File header structure (31 bytes fixed size)._ 
* **struct** [**star::FilePathInfo**](structstar_1_1FilePathInfo.md) 
* **struct** [**star::HotStorage**](structstar_1_1HotStorage.md) _Hot storage - frequently accessed data (cache-friendly)._ 
* **struct** [**star::IndexEntry**](structstar_1_1IndexEntry.md) _Index entry with block compression support and shape information._ 
* **struct** [**star::KeyRegistry**](structstar_1_1KeyRegistry.md) _Global key registry using data-oriented design (Structure of Arrays)._ 
* **struct** [**star::LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md) _Per-layer metadata registry using data-oriented design (Structure of Arrays)._ 
* **struct** [**star::MetadataValue**](structstar_1_1MetadataValue.md) _Type-erased wrapper for metadata values._ 
* **struct** [**star::OpenOptions**](structstar_1_1OpenOptions.md) _Read-time options for_ [_**StarDataset::open()**_](classstar_1_1StarDataset.md#function-open-12) _._
* **struct** [**star::S3Credentials**](structstar_1_1S3Credentials.md) _AWS Credentials with resolution chain._ 
* **struct** [**star::S3EndpointConfig**](structstar_1_1S3EndpointConfig.md) _S3 endpoint resolution (default AWS, or an override for S3-compatible services such as MinIO and for local testing)._ 
* **struct** [**star::S3Writer::HeaderData**](structstar_1_1S3Writer_1_1HeaderData.md) 
* **struct** [**star::Slice**](structstar_1_1Slice.md) _Describes a slice along one dimension (Python-style slicing) Plain struct - no methods except helpers, just data._ 
* **struct** [**star::SliceSpec**](structstar_1_1SliceSpec.md) _Complete slice specification for n-dimensional array._ 
* **struct** [**star::StarConfig**](structstar_1_1StarConfig.md) _Configuration for metadata block optimization._ 
* **struct** [**star::TypeToDataType**](structstar_1_1TypeToDataType.md) 
* **struct** [**star::TypeToDataType&lt; double &gt;**](structstar_1_1TypeToDataType_3_01double_01_4.md) 
* **struct** [**star::TypeToDataType&lt; float &gt;**](structstar_1_1TypeToDataType_3_01float_01_4.md) 
* **struct** [**star::TypeToDataType&lt; int16\_t &gt;**](structstar_1_1TypeToDataType_3_01int16__t_01_4.md) 
* **struct** [**star::TypeToDataType&lt; int32\_t &gt;**](structstar_1_1TypeToDataType_3_01int32__t_01_4.md) 
* **struct** [**star::TypeToDataType&lt; int64\_t &gt;**](structstar_1_1TypeToDataType_3_01int64__t_01_4.md) 
* **struct** [**star::TypeToDataType&lt; int8\_t &gt;**](structstar_1_1TypeToDataType_3_01int8__t_01_4.md) 
* **struct** [**star::TypeToDataType&lt; std::string &gt;**](structstar_1_1TypeToDataType_3_01std_1_1string_01_4.md) 
* **struct** [**star::TypeToDataType&lt; uint16\_t &gt;**](structstar_1_1TypeToDataType_3_01uint16__t_01_4.md) 
* **struct** [**star::TypeToDataType&lt; uint32\_t &gt;**](structstar_1_1TypeToDataType_3_01uint32__t_01_4.md) 
* **struct** [**star::TypeToDataType&lt; uint64\_t &gt;**](structstar_1_1TypeToDataType_3_01uint64__t_01_4.md) 
* **struct** [**star::TypeToDataType&lt; uint8\_t &gt;**](structstar_1_1TypeToDataType_3_01uint8__t_01_4.md) 
* **struct** [**star::AWSTokenCache::SSOToken**](structstar_1_1AWSTokenCache_1_1SSOToken.md) 
* **class** **std::enable_shared_from_this< StarDataset >**    
    * **class** [**star::StarDataset**](classstar_1_1StarDataset.md) _A cloud-optimized binary key-value store for serializable data types._ 

