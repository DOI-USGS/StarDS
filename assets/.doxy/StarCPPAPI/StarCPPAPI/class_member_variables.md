
# Class Member Variables



## a

* **active** ([**star::ByteUnshuffleSpec**](structstar_1_1ByteUnshuffleSpec.md))
* **access\_key** ([**star::S3Credentials**](structstar_1_1S3Credentials.md))
* **arena\_chunk\_size** ([**star::StarConfig**](structstar_1_1StarConfig.md))
* **access\_token** ([**star::AWSTokenCache::SSOToken**](structstar_1_1AWSTokenCache_1_1SSOToken.md))


## b

* **block\_indices** ([**star::BlockMap**](structstar_1_1BlockMap.md))
* **block\_offsets** ([**star::BlockMap**](structstar_1_1BlockMap.md))
* **block\_sizes** ([**star::BlockMap**](structstar_1_1BlockMap.md), [**star::LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md))
* **blocks\_contiguous** ([**star::BlockMap**](structstar_1_1BlockMap.md))
* **block\_size** ([**star::ByteUnshuffleSpec**](structstar_1_1ByteUnshuffleSpec.md), [**star::IndexEntry**](structstar_1_1IndexEntry.md), [**star::StarConfig**](structstar_1_1StarConfig.md))
* **blocked** ([**star::ByteUnshuffleSpec**](structstar_1_1ByteUnshuffleSpec.md))
* **block\_infos** ([**star::ColdStorage**](structstar_1_1ColdStorage.md))
* **block\_idx** ([**star::ExtractionPlan::ElementRange**](structstar_1_1ExtractionPlan_1_1ElementRange.md))
* **bucket** ([**star::FilePathInfo**](structstar_1_1FilePathInfo.md))
* **blocks** ([**star::IndexEntry**](structstar_1_1IndexEntry.md))
* **block\_positions** ([**star::LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md))
* **bucket\_region** ([**star::S3Writer::HeaderData**](structstar_1_1S3Writer_1_1HeaderData.md))
* **buffer\_shrink\_threshold** ([**star::StarConfig**](structstar_1_1StarConfig.md))


## c

* **compressed\_size** ([**star::BlockInfo**](structstar_1_1BlockInfo.md))
* **contiguous\_start\_offset** ([**star::BlockMap**](structstar_1_1BlockMap.md))
* **compressed\_sizes** ([**star::ColdStorage**](structstar_1_1ColdStorage.md))
* **compressions** ([**star::ColdStorage**](structstar_1_1ColdStorage.md), [**star::LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md))
* **compression** ([**star::IndexEntry**](structstar_1_1IndexEntry.md), [**star::StarConfig**](structstar_1_1StarConfig.md))
* **condition** ([**star::ThreadPool**](classstar_1_1ThreadPool.md))


## d

* **data\_indices** ([**star::HotStorage**](structstar_1_1HotStorage.md))
* **dirty\_flags** ([**star::HotStorage**](structstar_1_1HotStorage.md))
* **dtypes** ([**star::HotStorage**](structstar_1_1HotStorage.md))
* **datatype** ([**star::IndexEntry**](structstar_1_1IndexEntry.md))
* **dirty** ([**star::IndexEntry**](structstar_1_1IndexEntry.md))
* **data** ([**star::MetadataValue**](structstar_1_1MetadataValue.md))
* **dtype** ([**star::MetadataValue**](structstar_1_1MetadataValue.md))


## e

* **elem\_size** ([**star::ByteUnshuffleSpec**](structstar_1_1ByteUnshuffleSpec.md))
* **element\_count** ([**star::ExtractionPlan::ElementRange**](structstar_1_1ExtractionPlan_1_1ElementRange.md))
* **element\_start** ([**star::ExtractionPlan::ElementRange**](structstar_1_1ExtractionPlan_1_1ElementRange.md))
* **entry\_count** ([**star::FileHeader**](structstar_1_1FileHeader.md))
* **endpoint** ([**star::S3EndpointConfig**](structstar_1_1S3EndpointConfig.md))
* **element\_size** ([**star::SliceSpec**](structstar_1_1SliceSpec.md))
* **expires\_at** ([**star::AWSTokenCache::SSOToken**](structstar_1_1AWSTokenCache_1_1SSOToken.md))


## f

* **file\_positions** ([**star::ColdStorage**](structstar_1_1ColdStorage.md))
* **format\_version** ([**star::FileHeader**](structstar_1_1FileHeader.md))


## h

* **header\_size** ([**star::FileHeader**](structstar_1_1FileHeader.md))
* **hash\_to\_index** ([**star::KeyRegistry**](structstar_1_1KeyRegistry.md))
* **hashes** ([**star::KeyRegistry**](structstar_1_1KeyRegistry.md))


## i

* **is\_metadata\_block** ([**star::IndexEntry**](structstar_1_1IndexEntry.md))
* **is\_contiguous** ([**star::SliceSpec**](structstar_1_1SliceSpec.md))
* **is\_full\_rows** ([**star::SliceSpec**](structstar_1_1SliceSpec.md))


## k

* **key\_registry\_count** ([**star::FileHeader**](structstar_1_1FileHeader.md))
* **key** ([**star::FilePathInfo**](structstar_1_1FilePathInfo.md))
* **keys** ([**star::HotStorage**](structstar_1_1HotStorage.md))
* **key\_indices** ([**star::LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md))


## l

* **layer\_count** ([**star::FileHeader**](structstar_1_1FileHeader.md))
* **loaded\_flags** ([**star::HotStorage**](structstar_1_1HotStorage.md))
* **locations** ([**star::HotStorage**](structstar_1_1HotStorage.md))
* **layer\_names** ([**star::LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md))
* **layer\_inheritance** ([**star::OpenOptions**](structstar_1_1OpenOptions.md))


## m

* **m\_profiles** ([**star::AWSConfigParser**](classstar_1_1AWSConfigParser.md))
* **m\_access\_key** ([**star::AWSV4Signer**](classstar_1_1AWSV4Signer.md))
* **m\_region** ([**star::AWSV4Signer**](classstar_1_1AWSV4Signer.md), [**star::S3RangeReader**](classstar_1_1S3RangeReader.md), [**star::S3Writer**](classstar_1_1S3Writer.md))
* **m\_secret\_key** ([**star::AWSV4Signer**](classstar_1_1AWSV4Signer.md))
* **m\_session\_token** ([**star::AWSV4Signer**](classstar_1_1AWSV4Signer.md))
* **magic** ([**star::FileHeader**](structstar_1_1FileHeader.md))
* **m\_cached** ([**star::HttpRangeReader**](classstar_1_1HttpRangeReader.md), [**star::LocalRangeReader**](classstar_1_1LocalRangeReader.md), [**star::S3RangeReader**](classstar_1_1S3RangeReader.md))
* **m\_curl** ([**star::HttpRangeReader**](classstar_1_1HttpRangeReader.md), [**star::S3RangeReader**](classstar_1_1S3RangeReader.md), [**star::S3Writer**](classstar_1_1S3Writer.md))
* **m\_sink** ([**star::HttpRangeReader**](classstar_1_1HttpRangeReader.md), [**star::S3RangeReader**](classstar_1_1S3RangeReader.md))
* **m\_total\_size** ([**star::HttpRangeReader**](classstar_1_1HttpRangeReader.md), [**star::S3RangeReader**](classstar_1_1S3RangeReader.md))
* **m\_url** ([**star::HttpRangeReader**](classstar_1_1HttpRangeReader.md))
* **m\_whole** ([**star::HttpRangeReader**](classstar_1_1HttpRangeReader.md), [**star::LocalRangeReader**](classstar_1_1LocalRangeReader.md), [**star::S3RangeReader**](classstar_1_1S3RangeReader.md))
* **m\_layer\_name** ([**star::LayerMetadataAccessor**](classstar_1_1LayerMetadataAccessor.md), [**star::LayerView**](classstar_1_1LayerView.md))
* **m\_store** ([**star::LayerMetadataAccessor**](classstar_1_1LayerMetadataAccessor.md), [**star::MetadataAccessor**](classstar_1_1MetadataAccessor.md))
* **m\_base** ([**star::LayerView**](classstar_1_1LayerView.md))
* **meta** ([**star::LayerView**](classstar_1_1LayerView.md), [**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_path** ([**star::LocalRangeReader**](classstar_1_1LocalRangeReader.md))
* **m\_bytes** ([**star::MemoryRangeReader**](classstar_1_1MemoryRangeReader.md))
* **m\_data** ([**star::NDArray**](classstar_1_1NDArray.md))
* **m\_shape** ([**star::NDArray**](classstar_1_1NDArray.md))
* **m\_strides** ([**star::NDArray**](classstar_1_1NDArray.md))
* **m\_bucket** ([**star::S3RangeReader**](classstar_1_1S3RangeReader.md), [**star::S3Writer**](classstar_1_1S3Writer.md))
* **m\_creds** ([**star::S3RangeReader**](classstar_1_1S3RangeReader.md))
* **m\_key** ([**star::S3RangeReader**](classstar_1_1S3RangeReader.md), [**star::S3Writer**](classstar_1_1S3Writer.md))
* **m\_redirect\_region** ([**star::S3RangeReader**](classstar_1_1S3RangeReader.md))
* **m\_signer** ([**star::S3RangeReader**](classstar_1_1S3RangeReader.md), [**star::S3Writer**](classstar_1_1S3Writer.md))
* **metadata\_block\_enabled** ([**star::StarConfig**](structstar_1_1StarConfig.md))
* **metadata\_compression** ([**star::StarConfig**](structstar_1_1StarConfig.md))
* **metadata\_force\_separate\_keys** ([**star::StarConfig**](structstar_1_1StarConfig.md))
* **metadata\_max\_block\_size** ([**star::StarConfig**](structstar_1_1StarConfig.md))
* **m\_capture\_image** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_cold** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_compress\_buffer** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_config** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_data\_storage** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_file\_header** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_file\_mode** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_filename** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_flushed** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_header\_dirty** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_header\_size** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_hot** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_key\_registry** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_key\_to\_index** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_layer\_metadata\_indices** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_layer\_metadata\_loaded** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_layer\_metadata\_registry** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_layer\_presence** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_memory\_source** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_metadata\_dtypes** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_metadata\_loaded** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_metadata\_shapes** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_mutex** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_open\_options** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_path\_info** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_reader** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_s3\_credentials** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_serialize\_buffer** ([**star::StarDataset**](classstar_1_1StarDataset.md))
* **m\_thread\_pool** ([**star::StarDataset**](classstar_1_1StarDataset.md))


## n

* **name\_to\_index** ([**star::KeyRegistry**](structstar_1_1KeyRegistry.md))
* **names** ([**star::KeyRegistry**](structstar_1_1KeyRegistry.md))
* **name\_to\_layer\_index** ([**star::LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md))


## o

* **offset** ([**star::BlockInfo**](structstar_1_1BlockInfo.md))
* **output\_offset** ([**star::ExtractionPlan::ElementRange**](structstar_1_1ExtractionPlan_1_1ElementRange.md))
* **output\_shape** ([**star::SliceSpec**](structstar_1_1SliceSpec.md))


## p

* **path** ([**star::FilePathInfo**](structstar_1_1FilePathInfo.md))
* **position** ([**star::IndexEntry**](structstar_1_1IndexEntry.md))
* **prefetch\_whole\_below\_bytes** ([**star::OpenOptions**](structstar_1_1OpenOptions.md))
* **path\_style** ([**star::S3EndpointConfig**](structstar_1_1S3EndpointConfig.md))


## q

* **queue\_mutex** ([**star::ThreadPool**](classstar_1_1ThreadPool.md))


## r

* **ranges** ([**star::ExtractionPlan**](structstar_1_1ExtractionPlan.md))
* **region** ([**star::FilePathInfo**](structstar_1_1FilePathInfo.md), [**star::AWSTokenCache::SSOToken**](structstar_1_1AWSTokenCache_1_1SSOToken.md))


## s

* **shapes** ([**star::ColdStorage**](structstar_1_1ColdStorage.md))
* **stored\_in\_metadata\_flags** ([**star::ColdStorage**](structstar_1_1ColdStorage.md))
* **shape** ([**star::IndexEntry**](structstar_1_1IndexEntry.md), [**star::MetadataValue**](structstar_1_1MetadataValue.md))
* **stored\_in\_metadata** ([**star::IndexEntry**](structstar_1_1IndexEntry.md))
* **secret\_key** ([**star::S3Credentials**](structstar_1_1S3Credentials.md))
* **session\_token** ([**star::S3Credentials**](structstar_1_1S3Credentials.md))
* **start** ([**star::Slice**](structstar_1_1Slice.md))
* **step** ([**star::Slice**](structstar_1_1Slice.md))
* **stop** ([**star::Slice**](structstar_1_1Slice.md), [**star::ThreadPool**](classstar_1_1ThreadPool.md))
* **slices** ([**star::SliceSpec**](structstar_1_1SliceSpec.md))
* **start\_url** ([**star::AWSTokenCache::SSOToken**](structstar_1_1AWSTokenCache_1_1SSOToken.md))


## t

* **total\_compressed\_bytes** ([**star::BlockMap**](structstar_1_1BlockMap.md))
* **total\_elements** ([**star::ExtractionPlan**](structstar_1_1ExtractionPlan.md), [**star::SliceSpec**](structstar_1_1SliceSpec.md))
* **type** ([**star::FilePathInfo**](structstar_1_1FilePathInfo.md))
* **total\_bytes** ([**star::IndexEntry**](structstar_1_1IndexEntry.md))
* **tasks** ([**star::ThreadPool**](classstar_1_1ThreadPool.md))


## u

* **uncompressed\_size** ([**star::BlockInfo**](structstar_1_1BlockInfo.md))
* **uncompressed\_sizes** ([**star::ColdStorage**](structstar_1_1ColdStorage.md))
* **use\_https** ([**star::S3EndpointConfig**](structstar_1_1S3EndpointConfig.md))


## v

* **value** ([**star::TypeToDataType&lt; double &gt;**](structstar_1_1TypeToDataType_3_01double_01_4.md), [**star::TypeToDataType&lt; float &gt;**](structstar_1_1TypeToDataType_3_01float_01_4.md), [**star::TypeToDataType&lt; int16\_t &gt;**](structstar_1_1TypeToDataType_3_01int16__t_01_4.md), [**star::TypeToDataType&lt; int32\_t &gt;**](structstar_1_1TypeToDataType_3_01int32__t_01_4.md), [**star::TypeToDataType&lt; int64\_t &gt;**](structstar_1_1TypeToDataType_3_01int64__t_01_4.md), [**star::TypeToDataType&lt; int8\_t &gt;**](structstar_1_1TypeToDataType_3_01int8__t_01_4.md), [**star::TypeToDataType&lt; std::string &gt;**](structstar_1_1TypeToDataType_3_01std_1_1string_01_4.md), [**star::TypeToDataType&lt; uint16\_t &gt;**](structstar_1_1TypeToDataType_3_01uint16__t_01_4.md), [**star::TypeToDataType&lt; uint32\_t &gt;**](structstar_1_1TypeToDataType_3_01uint32__t_01_4.md), [**star::TypeToDataType&lt; uint64\_t &gt;**](structstar_1_1TypeToDataType_3_01uint64__t_01_4.md), [**star::TypeToDataType&lt; uint8\_t &gt;**](structstar_1_1TypeToDataType_3_01uint8__t_01_4.md))


## w

* **workers** ([**star::ThreadPool**](classstar_1_1ThreadPool.md))




