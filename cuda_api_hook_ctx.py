from contextlib import contextmanager

@contextmanager
def cuda_api_hook_ctx(cuda_api_name="ON"):
    import os
    key = "CUDA_API_HOOK_CTX"
    default_value = "OFF"
    original_value = os.environ.get(key, default_value)
    
    try:
        os.environ[key] = cuda_api_name
        print(f'------------[CUDA API HOOK] begin hook {cuda_api_name}')
        yield
    finally:
        print(f'------------[CUDA API HOOK] end   hook {cuda_api_name}')
        os.environ[key] = original_value

class CudaApiHookCtx:
    _original_value = "OFF"
    _key = "CUDA_API_HOOK_CTX"
    _default_value = "OFF"
    _cuda_api_name = "ON"

    @classmethod
    def begin(cls, cuda_api_name="ON"):
        cls._cuda_api_name = cuda_api_name
        import os
        cls._original_value = os.environ.get(cls._key, cls._default_value)
        os.environ[cls._key] = cls._cuda_api_name
        print(f'------------[CUDA API HOOK] begin hook {cls._cuda_api_name}')

    @classmethod
    def end(cls):
        import os
        os.environ[cls._key] = cls._original_value
        print(f'------------[CUDA API HOOK] end   hook {cls._cuda_api_name}')

if __name__ == "__main__":
    import os
    key = "CUDA_API_HOOK_CTX"
    print(f'begin {os.environ.get(key, "UNSET")=}')
    with cuda_api_hook_ctx():
        print(f'in {os.environ.get(key, "UNSET")=}')
    print(f'end {os.environ.get(key, "UNSET")=}')
    with cuda_api_hook_ctx():
        print(f'in {os.environ.get(key, "UNSET")=}')
    print(f'end {os.environ.get(key, "UNSET")=}')
    CudaApiHookCtx.begin()
    print(f'in {os.environ.get(key, "UNSET")=}')
    CudaApiHookCtx.end()
    print(f'end {os.environ.get(key, "UNSET")=}')
    with cuda_api_hook_ctx("cudaMalloc"):
        print(f'in {os.environ.get(key, "UNSET")=}')
    print(f'end {os.environ.get(key, "UNSET")=}')
    with cuda_api_hook_ctx("cudaFree"):
        print(f'in {os.environ.get(key, "UNSET")=}')
    print(f'end {os.environ.get(key, "UNSET")=}')
    CudaApiHookCtx.begin("cudaMalloc")
    print(f'end {os.environ.get(key, "UNSET")=}')
    CudaApiHookCtx.end()
    print(f'end {os.environ.get(key, "UNSET")=}')
