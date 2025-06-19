# cuda_api_hook  

中文  

---  

作用类似于 python 函数装饰器，只不过作用于 cuda api，其中一个应用是 record kernel。

# 使用方法

## 以 record kernel 为例

编译 hook so ：  
```python  
g++ -shared -fPIC -o cuda_api_hook.so cuda_api_hook.cpp -ldl -I/usr/local/cuda-12.8/targets/x86_64-linux/include/  
```
在项目启动脚本中 export 环境变量：  
```python  
export LD_PRELOAD=/your_so_dir/cuda_api_hook.so  
```  
记得将 your_so_dir 替换为实际的目录。  
将 cuda api hook 上下文对象作用于需要 record kernel 的 python 代码段：  
```python  
with cuda_api_hook_ctx("cudaLaunchKernel"):  
    # your python code  
```  
最后启动项目，就会打印执行的 kernel 相关的信息。  

English  

---  

Functionality similar to Python function decorators, but operating on CUDA APIs, with one application being kernel recording.  

# Usage  

## Example: Recording Kernels  

Compile hook shared object:  
```python  
g++ -shared -fPIC -o cuda_api_hook.so cuda_api_hook.cpp -ldl -I/usr/local/cuda-12.8/targets/x86_64-linux/include/  
```
Export environment variable in project startup script:   
```python  
export LD_PRELOAD=/your_so_dir/cuda_api_hook.so  
```  
Remember to replace 'your_so_dir' with the actual directory.  
context to Python code segments requiring kernel recording:  
```python  
with cuda_api_hook_ctx("cudaLaunchKernel"):  
    # your python code  
```  
Finally, launch the project and it will print information about the executed kernels.