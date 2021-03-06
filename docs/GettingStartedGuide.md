# Getting Started Guide

## Prerequisites

To start using the HERE Open Location Platform (OLP) SDK for C++, you need an OLP user account.

To work with the HERE OLP SDK for C++, you need to understand the following concepts related to OLP:

- [Catalogs](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/catalogs.html)
- [Layers](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/layers.html)
- [Partitions](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/partitions.html)
- [HERE Resource Names (HRNs)](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/hrn.html)

For more information, see the [HERE OLP Data User Guide](https://developer.here.com/olp/documentation/data-user-guide/index.html).

## Get Credentials

To work with catalog or service requests to the HERE Open Location Platform (OLP), you need to get authentication and authorization credentials.

You can authenticate to OLP within your application with the platform credentials available on the OLP portal by means of the HERE OLP SDK for C++ authentication library. For instructions on how to get credentials, see [the related section](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html) in the Terms and Permissions User Guide.

## Available Components

The HERE OLP SDK for C++ contains separate libraries, each of which has a distinct functionality. For more information about the components, see the [architectural overview](OverallArchitecture.md).

## HERE OLP SDK for C++ in CMake Projects

Once the libraries are installed, you can find them using the `find_package()` function within your project. For more information on how to install libraries, see [the related section](../README.md#install-the-sdk) in the README.md file.

```CMake
find_package(olp-cpp-sdk-core REQUIRED)
find_package(olp-cpp-sdk-authentication REQUIRED)
find_package(olp-cpp-sdk-dataservice-read REQUIRED)
find_package(olp-cpp-sdk-dataservice-write REQUIRED)
```

Once the necessary targets are imported, you can link them to your library:

```CMake
target_link_libraries(example_app
    olp-cpp-sdk-core
    olp-cpp-sdk-authentication
    olp-cpp-sdk-dataservice-read
    olp-cpp-sdk-dataservice-write
)
```

## Reference Documentation

The API reference documentation for the HERE OLP SDK for C++ is available on our [GitHub Pages](https://heremaps.github.io/here-olp-sdk-cpp/).

## Examples

To better understand the OLP SDK for C++, see the following examples:

- [Read example](dataservice-read-catalog-example.md) &ndash; shows how to get catalog and partition metadata, as well as partition data.
- [Write example](dataservice-write-example.md) &ndash; shows how to publish data to a specific stream layer partition.
