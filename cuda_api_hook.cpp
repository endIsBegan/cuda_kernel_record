#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <cxxabi.h>

#include <string>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <cctype>
#include <utility>

#include <cuda_runtime.h>

enum class DataType {INT = 0, FLOAT, BOOL, PTR, DOUBLE, SIZE_T, UINT, UNREC};
using TypeInfo = std::pair<size_t, DataType>;

#define CHECK_CUDA_ERROR(call) \
    do { \
        cudaError_t err = (call); \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ << ": " \
                      << cudaGetErrorString(err) << std::endl; \
        } \
    } while(0)

std::string format_void_ptr(int index, void* ptr, DataType type) {
    std::ostringstream oss;
    oss << "(" << index << ",";
    switch (type) {
        case DataType::INT:
            oss << "int," << *static_cast<int*>(ptr);
            break;
        case DataType::FLOAT:
            oss << "float," << std::fixed << std::setprecision(2) << *static_cast<float*>(ptr);
            break;
        case DataType::BOOL:
            oss << "bool," << std::boolalpha << *static_cast<bool*>(ptr);
            break;
        case DataType::PTR:
            oss << "ptr," << *static_cast<size_t *>(ptr);
            break;
        case DataType::DOUBLE:
            oss << "double," << std::fixed << std::setprecision(2) << *static_cast<double*>(ptr);
            break;
        case DataType::SIZE_T:
            oss << "size_t," << *static_cast<size_t*>(ptr);
            break;
        case DataType::UINT:
            oss << "uint," << *static_cast<unsigned int*>(ptr);
            break;
        default:
            oss << "??";
            break;
    }
    oss << ")";
    return oss.str();
}

std::string format_void_array(void** data_array, const std::vector<TypeInfo>& type_info) {
    std::string result;
    for (size_t i = 0; i < type_info.size(); ++i) {
        const auto& [index, type] = type_info[i];
        if (!result.empty()) {
            result += ",";
        }
        if(i != 0 && i % 4 == 0){
            result += "\n  ";
        }
        result += format_void_ptr(index, data_array[index], type);
    }
    return result;
}

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" ");
    auto end = str.find_last_not_of(" ");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

std::vector<TypeInfo> parseFunctionParamsFmt(const std::string& demangled_decl) {
    std::vector<TypeInfo> result;
    
    size_t start_paren = demangled_decl.find('(');
    size_t end_paren = demangled_decl.rfind(')');
    
    if (start_paren == std::string::npos || end_paren == std::string::npos || 
        start_paren >= end_paren) {
        return result; // invalid func dec
    }
    std::string params_str = demangled_decl.substr(start_paren + 1, end_paren - start_paren - 1);
    std::istringstream iss(params_str);
    std::string type_str;
    size_t index = 0;
    
    while (std::getline(iss, type_str, ',')) {
        type_str = trim(type_str);
        if (type_str.empty()) continue;
        
        if (type_str.find('*') != std::string::npos) {
            result.emplace_back(index++, DataType::PTR);
            continue;
        }
        std::transform(type_str.begin(), type_str.end(), type_str.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (type_str == "int" || type_str == "long" || type_str == "signed") {
            result.emplace_back(index++, DataType::INT);
        } else if (type_str == "float") {
            result.emplace_back(index++, DataType::FLOAT);
        } else if (type_str == "double") {
            result.emplace_back(index++, DataType::DOUBLE);
        } else if (type_str == "bool") {
            result.emplace_back(index++, DataType::BOOL);
        }else if (type_str == "unsigned int" || type_str == "unsigned" || type_str == "uint32_t") {
            result.emplace_back(index++, DataType::UINT);
        } else if (type_str == "size_t" || type_str == "uint64_t") {
            result.emplace_back(index++, DataType::SIZE_T);
        } else {
            result.emplace_back(index++, DataType::UNREC);
        }
    }
    return result;
}

std::string extractFuncName(const std::string& decl) {
    size_t param_start = decl.find('(');
    if (param_start == std::string::npos) {
        return "";
    }

    std::string prefix = decl.substr(0, param_start);
    auto end_it = std::find_if_not(prefix.rbegin(), prefix.rend(), 
        [](char c) { return std::isspace(c); });
    prefix = (end_it == prefix.rend()) ? "" : prefix.substr(0, end_it.base() - prefix.begin());

    int bracket_count = 0;
    auto name_start = std::find_if(prefix.rbegin(), prefix.rend(), 
        [&](char c) {
            if (c == '>') bracket_count++;
            else if (c == '<' && bracket_count > 0) bracket_count--;
            return bracket_count == 0 && std::isspace(c);
        });

    return (name_start == prefix.rend()) 
        ? prefix 
        : std::string(name_start.base(), prefix.end());
}

std::string demangle(const std::string& mangle_name) {
    int status = 0;
    std::string demangled = abi::__cxa_demangle(mangle_name.c_str(), nullptr, nullptr, &status);
    if (status != 0) {
        std::cerr << "demangle err " << mangle_name << std::endl;
        return "??";
    }
    return demangled;
}

std::string DataTypeToStr(DataType dataType){
    std::string dataTypeStr;
    switch (dataType) {
        case DataType::INT:
            dataTypeStr = "int";;
            break;
        case DataType::FLOAT:
            dataTypeStr = "float";
            break;
        case DataType::BOOL:
            dataTypeStr = "bool";
            break;
        case DataType::PTR:
            dataTypeStr = "ptr";
            break;
        case DataType::DOUBLE:
            dataTypeStr = "double";
            break;
        case DataType::SIZE_T:
            dataTypeStr = "size_t";
            break;
        case DataType::UINT:
            dataTypeStr = "uint";
            break;
        default:
            dataTypeStr = "??";
            break;
    }
    return dataTypeStr;
}

void get_params(const std::string &mangle_func_dec, void **params, std::string &func_name, std::string &params_str){
    static std::map<std::string, std::pair<std::string, std::vector<TypeInfo>>> func_infos = {
        {"??", {"??", {}}}
        //you can add something
    };
    if (func_infos.find(mangle_func_dec) == func_infos.end()) {
        std::string demangle_func_dec = demangle(mangle_func_dec);
        func_name = extractFuncName(demangle_func_dec);
        func_infos.emplace(mangle_func_dec, std::make_pair(func_name, parseFunctionParamsFmt(demangle_func_dec)));
    }
    func_name = func_infos[mangle_func_dec].first;
    params_str = format_void_array(params, func_infos[mangle_func_dec].second);
}

std::string load_func_name(const void *func){
    std::string func_name = "??";
    Dl_info info;
    if (dladdr(func, &info)) {
        func_name = info.dli_sname;
        // std::cout << "File path: " << info.dli_fname << std::endl;
    } else {
        std::cerr << " Failed to resolve function name." << std::endl;
    }
    return func_name;
}

bool isInContext(const char *func_name){
    const char* env_var = std::getenv("CUDA_API_HOOK_CTX");
    bool cuda_api_hook_ctx_on = false;
    if (env_var) {
        if (std::strcmp(env_var, "ON") == 0) {
            cuda_api_hook_ctx_on = true;
        } else {
            cuda_api_hook_ctx_on = false;
        }
    }
    if(env_var && !cuda_api_hook_ctx_on){
        if(std::strcmp(env_var, func_name) == 0) {
            cuda_api_hook_ctx_on = true;
        }
    }
    return cuda_api_hook_ctx_on;
}

template<typename T>
T load_func(const std::string &func_name, const std::string &so_name){
    T func_ptr = nullptr;
    static char buf[128];
    // load so
    void *handle = dlopen(so_name.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        std::cerr << " Error in dlopen: " << dlerror() << std::endl;
        return nullptr;
    }
    func_ptr = reinterpret_cast<T>(dlsym(handle, func_name.c_str()));
    if (!func_ptr) {
        std::cerr << " Error in dlsym: " << dlerror() << std::endl;
        return nullptr;
    }
    return func_ptr;
}

//ref paddle
struct CUDAGraphCaptureModeGuard {
    explicit CUDAGraphCaptureModeGuard(cudaStreamCaptureMode mode = cudaStreamCaptureModeRelaxed) {
        CHECK_CUDA_ERROR(cudaThreadExchangeStreamCaptureMode(&mode));
        old_mode_ = mode;
    }

    ~CUDAGraphCaptureModeGuard() {
        CHECK_CUDA_ERROR(cudaThreadExchangeStreamCaptureMode(&old_mode_));
    }
    cudaStreamCaptureMode old_mode_;
};

typedef cudaError_t (*cudaMalloc_t)(void **devPtr, size_t size);
typedef cudaError_t (*cudaFree_t)(void *devPtr);
typedef cudaError_t (*cudaLaunchKernel_t)(const void *func, dim3 gridDim, dim3 blockDim, void **args, size_t sharedMem, cudaStream_t stream);

extern "C" {
    
    //cudaMalloc hook
    cudaError_t cudaMalloc(void **devPtr, size_t size){
        // real target func
        static cudaMalloc_t real_cudaMalloc = nullptr;

        if (!real_cudaMalloc) {
            real_cudaMalloc = load_func<cudaMalloc_t>("cudaMalloc", "libcudart.so");
            if(!real_cudaMalloc) return cudaErrorInvalidValue;
        }
        cudaError_t err = real_cudaMalloc(devPtr, size);
        if(isInContext("cudaMalloc")){
            std::cout << "  call cudaMalloc (" <<  *(size_t *)devPtr << "," << size << ")"<< std::endl;
        }
        return err;
    }

    //cudaFree hook
    cudaError_t cudaFree(void *devPtr){
        // real target func
        static cudaFree_t real_cudaFree = nullptr;

        if (!real_cudaFree) {
            real_cudaFree = load_func<cudaFree_t>("cudaFree", "libcudart.so");
            if(!real_cudaFree) return cudaErrorInvalidValue;
        }
        cudaError_t err = real_cudaFree(devPtr);
        if(isInContext("cudaFree")){
            //do something
            std::cout << "  call cudaFree " << size_t(devPtr) << std::endl;
        }
        return err;
    }

    //cudaLaunchKernel hook
    cudaError_t cudaLaunchKernel(const void *func, dim3 gridDim, dim3 blockDim, void **args, size_t sharedMem, cudaStream_t stream) {
        // real target func
        static cudaLaunchKernel_t real_cudaLaunchKernel = nullptr;
        if (!real_cudaLaunchKernel) {
            real_cudaLaunchKernel = load_func<cudaLaunchKernel_t>("cudaLaunchKernel", "libcudart.so");
            if(!real_cudaLaunchKernel) return cudaErrorInvalidValue;
        }
        if(isInContext("cudaLaunchKernel")){
            std::ostringstream oss;
            auto mangle_func_dec = load_func_name(func);
            std::string func_name, params_str;
            get_params(mangle_func_dec, args, func_name, params_str);
            
            oss << " " << func_name << "\n  <<<(" << gridDim.x << "," << gridDim.y << "," << gridDim.z << "),"
                << "(" << blockDim.x << "," << blockDim.y << "," << blockDim.z << "),(" << sharedMem << "),(" 
                << size_t(stream) << ")>>>\n  " << params_str << std::endl;
            std::cout << oss.str() << std::endl;
        }
        cudaError_t err = real_cudaLaunchKernel(func, gridDim, blockDim, args, sharedMem, stream); 
        return err;
    }

}// end extern "C"

/*ref cmd

g++ -shared -fPIC -o cuda_api_hook.so cuda_api_hook.cpp -ldl -I/usr/local/cuda-12.8/targets/x86_64-linux/include/
export LD_PRELOAD=/your_so_dir/cuda_api_hook.so

*/