
# I'm worried that naming this library "openssl" will conflict with the
# system C library named OpenSSL, so I'll name it "openssl_wrapper" to be safe
ADD_LIBRARY(openssl_wrapper OBJECT hash.cpp openssl_exception.cpp signature.cpp envelope_key.cpp envelope_encryption.cpp blind_rsa.c blind_signature.cpp)
target_include_directories(openssl_wrapper PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>
)
