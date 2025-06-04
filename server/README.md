# 服务端代码

这个文件夹用于存放服务端代码。将本目录下的 `main.py` 打包成exe文件并在服务端运行，读取服务器的状态。

## notes

1. 监听的网卡设备名需要自行修改。

2. 安装 `pyinstaller`，使用命令行将代码打包成 exe:

```cmd
pyinstaller --onefile main.py
```

有时候 flask 依赖打包不进去，需要手动添加

``` cmd
pyinstaller --onefile --collect-all flask main.py
```
3. 需要保持 `main.exe` 一直在服务器后台运行
