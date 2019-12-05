## HotPatcher参数说明

![image-20191205163654635](C:\Users\imzlp\AppData\Roaming\Typora\typora-user-images\image-20191205163654635.png)

### BaseVersion

该选项应选择Patch所基于的版本文件，可以`ByRelease`或者上次一的Patch生成，默认为`*_Release.json`。

![image-20191205163724262](C:\Users\imzlp\AppData\Roaming\Typora\typora-user-images\image-20191205163724262.png)

### VersionId

当前Patch的版本的ID

### IncludeFilter

当前Patch扫描哪些目录下的资源变动。

### IgnoreFilter

当前Patch忽略哪些目录下的资源变动。

### IncludeAssetRegistry

在当前的Patch打出的Pak中包含`AssetRegistry.bin`文件。

### IncludeGlobalShaderCache

在当前的Patch打出的Pak中包含`GlobalShaderCache-*.bin`文件。

### IncludeProjectIni

在当前的Patch打出的Pak中包含项目的ini文件（不会包含`DefaultEditor*.ini`）

### AddExternFileToPak

添加外部的非资源文件到Pak中，如txt、视频。

1. `FilePath`需要指定所选文件的路径。
2. `MountPath`为该文件被打包到Pak中的挂载路径，默认是`../../../PROJECT_NAME/`下。

在游戏运行时可以通过`FPaths::ProjectDir`来访问。

如，AAAA.json的`MountPath`为`../../../HotPatcherExample/AAAAA.json`，在运行时加载的路径：

```
FPaths::Combine(FPaths::ProjectDir(),TEXT("AAAAAA.json"));
```

Pak中的所有文件可以通过`IPlatformFile`来访问。

### UnrealPakOptions

该数组为`UnrealPak.exe`程序的参数。如果什么都不指定，默认的配置为：

```bash
UnrealPak.exe PAKFILE.pak -create=PAK_LIST.txt
```

### PakTargetPlatforms

该数组为选择要打出的Patch的平台，可以多选，一定要注意所选的平台已经被Cook。

### SavePakList

是否存储`UnrealPak.exe` 的`-Create`参数文件。

### SaveDiffAnalysis

是否存储当前的Patch版本与Base版本的差异信息。

### SavePatchConfig

是否存储当前Patch的所有选项信息。

### SavePath

本次的Patch信息存储位置。会在当前目录下创建出名字为`VersionId`的文件夹，所有的文件在此文件夹中。

