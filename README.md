## UE4 Plugin: HotPatcher

这个插件是我最近写的用于管理UE项目资源热更的工具，用于追踪版本间的资源变动来打出Patch。
与UnrealFrontEnd中的Patch不同，UE的Patch是基于Cook出的资产作为版本管理的内容，这样在管理工程时存在问题：同样的工程版本，很难在不同的电脑上打出相同的Patch（除非把Cooked同步提交），而且还无法基于Patch的版本再打出一个Patch，这个插件就是为了解决这样的问题。

另外，这个插件也支持只打包指定的资产（可以指定**包含过滤器**以及**忽略过滤器**以及可以**指定单个资源**），支持资源的递归依赖分析，打包的Path中不会包含未引用的资源，也不会包含未改动的资源，还会分析项目中的无效资产以及忽略重定向器的资源，实现了基于项目资产的版本追踪而无需管理Cooked的内容，只需要在打Patch之前Cook一遍保证Cook的内容是基于最新工程版本的即可。

除了包含UE的资源文件外，额外的支持：

- 支持包含Cooked出的非资源文件如`AssetRegistry.bin`/`GlobalShaderCache*.bin`/`ShaderBytecode*.ushaderbytecode`；
- 支持包含引擎、项目、和插件的ini文件；
- 支持包含外部的文件夹和文件（如lua文件，视频文件等）供运行时访问；

插件的其他功能：

- 支持检测未cook的资源；
- 支持版本间的diff，可以看到新增、修改、删除（但是删除的资源在之前的版本的pak中是无法删除的）；
- 支持检测重复的文件包含；
- 支持导出该插件所有的中间生成信息和配置；

同时，我也写了一个批量Cook的工具，用于一键Cook指定的多个平台（当然使用命令行也可以），目的就是使用最少的步骤完成任务。


### Cook参数说明

HotPatcher中的Cook部分是为了打Patch时方便Cook多个平台，以及方便指定地图，不用每次Cook都使用命令行。

![](https://imzlp.me/notes/index/UE4/Plugins/HotPatcher/HotPatcher-CookContent.png)

Cook可选的分为`Platforms`/`Map(s)`/`Settings`：

- `Platforms`：选择Cook 的平台，可以多选；
- `Map(s)`：选择要Cook的Map，该选项下会列出当前工程里所有的Map，可以多选；
- `Settings`：选择Cook设置，默认提供了`Iterator`/`UnVersioned`/`CookAll`/`Compressed`四个选项，我也提供了`OtherOptions`可以自己指定要Cook 的参数。

> 注意：`Map(s)`中的地图和`Settings`中的`CookAll`必须要选中其中的一个才可以打包，如果只选了CookMap，则只会Cook该Map所引用到的资源，没有被引用的不会被Cook，这个需要注意。

点击CookContent之后会将Cook的log输出到UE的`OutputLog`中。

### ByRelease参数说明

>ByRelease操作导出的是一个json的文件，记录了导出时所选择的每个Asset资源的HASH值，基于此HASH值我们可以在后续的Patch中知道该资源是不是被修改了。

![](https://imzlp.me/notes/index/UE4/Plugins/HotPatcher/HotPatcher-Export-Release.png)

- **VersionId**：当前导出的资源信息是什么版本。
- **IncludeFilter**：当前Patch扫描哪些目录下的资源变动。
- **IgnoreFilter**：当前Patch忽略哪些目录下的资源变动。
- **IncludeHasRefAssetsOnly**：对选中的过滤器中的资源文件进行依赖分析（递归分析至Map），如果资源没有被引用则不会打包到Patch中。
- **IncludeSpecifyAssets**：结构的数组，可以指定需要打到Pak中的单个资源，该结构的第一个参数需要指定资源，第二个参数控制是否分析并包含当前指定资源的依赖到pak中。

### ByPatch参数说明

> ByPatch是真正执行打包出Pak的工具，可以基于之前导出的版本(通过ByRelease导出的Json文件)，也可以包含外部文件/文件夹、配置文件（ini）、Cook出的非资源文件（AssetRegistry.bin等），并且可以指定多个平台，支持输入UnrealPak的参数，可以导出当前Patch的各种信息。

![](https://imzlp.me/notes/index/UE4/Plugins/HotPatcher/HotPatcher-Export-Patch.png)

支持指定资源过滤器：

![](https://imzlp.me/notes/index/UE4/Plugins/HotPatcher/HotPatcher-Export-Patch-Filter.png)

HotPatcher打Patch的选项解析：

- **bByBaseVersion**：是否是基于某个基础版本的Patch，若为`false`，则只打包选择的过滤器文件（依然会分析依赖）和添加的外部文件，若为`true`则必须要指定一个基础版本，否则无法执行Patch。同时该属性也会控制是否生成`Diff`信息，若为`false`则不生成`Diff`（没有基础版本diff也无意义）。
- **BaseVersion**：该选项应选择Patch所基于的版本文件，可以`ByRelease`或者上次一的Patch生成，默认为`*_Release.json`。
- **VersionId**：当前Patch的版本的ID
- **IncludeFilter**：当前Patch扫描哪些目录下的资源变动。
- **IgnoreFilter**：当前Patch忽略哪些目录下的资源变动。
- **IncludeHasRefAssetsOnly**：对选中的过滤器中的资源文件进行依赖分析（递归分析至Map），如果资源没有被引用则不会打包到Patch中。
- **IncludeSpecifyAssets**：结构的数组，可以指定需要打到Pak中的单个资源，该结构的第一个参数需要指定资源，第二个参数控制是否分析并包含当前指定资源的依赖到pak中。
- **IncludeAssetRegistry**：在当前的Patch打出的Pak中包含`AssetRegistry.bin`文件。
- **IncludeGlobalShaderCache**：在当前的Patch打出的Pak中包含`GlobalShaderCache-*.bin`文件。
- **IncludeShaderBytecode**：在当前Patch打出的Pak中包含`PROJECT_NAME\Content\ShaderArchive*.ushaderbytecode`文件。
- **IncludeEngineIni**：在当前打出的Patch中包含引擎目录下的ini，也会包含平台相关的ini
- **IncludePluginIni**：在当前打出的Patch中包含所有启用的插件中的ini（引擎目录和项目目录的插件都会包含）
- **IncludeProjectIni**：在当前的Patch打出的Pak中包含项目的ini文件（不会包含`DefaultEditor*.ini`）
- **AddExternFileToPak**：添加外部的非资源文件到Pak中，如txt、视频。

**AddExternFileToPak**的元素要求：

1. `FilePath`需要指定所选文件的路径。
2. `MountPath`为该文件被打包到Pak中的挂载路径，默认是`../../../PROJECT_NAME/`下。

在游戏运行时可以通过`FPaths::ProjectDir`来访问。
如，AAAA.json的`MountPath`为`../../../HotPatcherExample/AAAAA.json`，在运行时加载的路径：

```
FPaths::Combine(FPaths::ProjectDir(),TEXT("AAAAAA.json"));
```
Pak中的所有文件可以通过`IPlatformFile`来访问。

- **AddExternDirectoryToPak**：结构的数组，添加外部文件夹到Pak中，该结构第一个参数(`DirectoryPath`)为指定系统中的文件夹路径，第二个参数(`Mount Point`)指定该路径在Pak中的挂载路径；指定文件夹下的所有文件会被递归包含，并且挂载路径均相对于所指定的`MountPoint`。

- **IncludePakVersionFile**：是否在当前打出的Patch中存储版本信息。
- **PakVersionFileMountPoint**：由`IncludePakVersionFile`控制是否可以编辑，用于指定`*_PakVersion.json`文件在Pak文件中的挂载点，默认为`../../../PROJECT_NAME/Extention/Versions`。

> 如果选择了在Pak存储版本信息，则可以使用`FPakHelper::LoadVersionInfoByPak`来读取Pak中的版本信息，可以用在Pak文件验证上。

Pak的版本信息为以下结构：

```cpp
USTRUCT(BlueprintType)
struct FPakVersion
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString VersionId;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString BaseVersionId;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString Date;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString CheckCode;
};
```

其中：`VersionId`为Pak的版本Id，`BaseVersionId`为该Pak的基础版本ID，`Date`为Pak创建的日期，`CheckCode`的值默认为把`VersionId_BaseVersionId_Date`拼接到一块进行的SHA1编码。

- **UnrealPakOptions**：该数组为`UnrealPak.exe`程序的参数。如果什么都不指定，默认的配置为`UnrealPak.exe PAKFILE.pak -create=PAK_LIST.txt`.

- **PakTargetPlatforms**：该数组为选择要打出的Patch的平台，可以多选，一定要注意所选的平台已经被Cook。

- **SavePakList**：是否存储`UnrealPak.exe` 的`-Create`参数文件。

- **SaveDiffAnalysis**：是否存储当前的Patch版本与Base版本的差异信息。

- **SavePatchConfig**：是否存储当前Patch的所有选项信息。

- **SavePath**：本次的Patch信息存储位置。会在当前目录下创建出名字为`VersionId`的文件夹，所有的文件在此文件夹中。

### Update Log

#### 2019.12.08 Update

- 新增是否对资源进行依赖扫描的选项，增加diff预览。

![](https://imzlp.me/notes/index/UE4/Plugins/HotPatcher/HotPatcher-Diff-view.png)

#### 2020.01.13 Update

- 新增生成时的错误信息提示，对未Cook资源的扫描以及重复添加的外部文件进行检测，增加可以指定特定资源到Pak中，并且可选是否对该资源进行依赖分析。

![](https://imzlp.me/notes/index/UE4/Plugins/HotPatcher/HotPatcher-GeneratedPatch-ErrorMsg.png)

#### 2020.01.14 Update

- 修复会扫描到Redirector会提示Redirector的资源未Cooked，为ExportRelease增加`IncludeSpecifyAssets`，可以指定某个资源了（如只指定某个地图）。

![](https://imzlp.me/notes/index/UE4/Plugins/HotPatcher/HotPatcher-diff-specifty-assets.png)

