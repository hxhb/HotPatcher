//// Fill out your copyright notice in the Description page of Project Settings.
//
//
//#include "FlibAssetManageHelper.h"
//#include "SoftObjectPath.h"
//#include "Kismet/KismetStringLibrary.h"
//#include "Kismet/KismetSystemLibrary.h"
//#include "Json.h"
//#include "SharedPointer.h"
//#include "IPluginManager.h"
//
//#ifdef __DEVELOPER_MODE__
//#include "Interfaces/ITargetPlatform.h"
//#include "Interfaces/ITargetPlatformManagerModule.h"
//#endif
//
//void UFlibAssetManageHelper::GetAssetDependencies(const FString& InAssetRelativePath, FAssetDependenciesInfo& OutDependInfo)
//{
//	if (InAssetRelativePath.IsEmpty())
//		return;
//
//	FStringAssetReference AssetRef = FStringAssetReference(InAssetRelativePath);
//	if (!AssetRef.IsValid())
//		return;
//	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
//
//	FStringAssetReference InAssetRef = InAssetRelativePath;
//	FString TargetLongPackageName = InAssetRef.GetLongPackageName();
//	// UE_LOG(LogTemp, Log, TEXT("TargetLongPackageName is %s."), *TargetLongPackageName);
//	if (FPackageName::DoesPackageExist(TargetLongPackageName))
//	{
//		{
//			TArray<FAssetData> AssetDataList;
//			bool bResault = AssetRegistryModule.Get().GetAssetsByPackageName(FName(*TargetLongPackageName), AssetDataList);
//			if (!bResault || !AssetDataList.Num())
//			{
//				UE_LOG(LogTemp, Error, TEXT("Faild to Parser AssetData of %s, please check."), *TargetLongPackageName);
//				return;
//			}
//			if (AssetDataList.Num() > 1)
//			{
//				UE_LOG(LogTemp, Warning, TEXT("Got mulitple AssetData of %s,please check."), *TargetLongPackageName);
//			}
//		}
//		UFlibAssetManageHelper::GatherAssetDependicesInfoRecursively(AssetRegistryModule, TargetLongPackageName, OutDependInfo);
//	}
//
//}
//
//
//void UFlibAssetManageHelper::GetAssetListDependencies(const TArray<FString>& InAssetRelativePathList, FAssetDependenciesInfo& OutDependices)
//{
//	for (const auto& AssetItem : InAssetRelativePathList)
//	{
//		UFlibAssetManageHelper::GetAssetDependencies(AssetItem, OutDependices);
//	}
//}
//void UFlibAssetManageHelper::GetAssetDependenciesBySoftRef(TSoftObjectPtr<UObject> InAssetRef, FAssetDependenciesInfo& OutDependicesInfo)
//{
//	if (!InAssetRef.IsValid())
//		return;
//	UFlibAssetManageHelper::GetAssetDependencies(InAssetRef.ToString(), OutDependicesInfo);
//
//}
//
//TArray<FString> UFlibAssetManageHelper::GetAssetAllDependencies(const FString& InAssetRelativePath)
//{
//	TArray<FString> Resault;
//	FAssetDependenciesInfo local_DependenciesInfo;
//	GetAssetDependencies(InAssetRelativePath, local_DependenciesInfo);
//
//	TArray<FString> Keys;
//	local_DependenciesInfo.mDependencies.GetKeys(Keys);
//	for (auto& Key : Keys)
//	{
//		Resault.Append(local_DependenciesInfo.mDependencies[Key].mDependAsset);
//	}
//	return Resault;
//}
//
//void UFlibAssetManageHelper::GatherAssetDependicesInfoRecursively(
//	FAssetRegistryModule& InAssetRegistryModule,
//	const FString& InTargetLongPackageName,
//	FAssetDependenciesInfo& OutDependencies
//)
//{
//	TArray<FName> local_Dependencies;
//	bool bGetDependenciesSuccess = InAssetRegistryModule.Get().GetDependencies(FName(*InTargetLongPackageName), local_Dependencies, EAssetRegistryDependencyType::Packages);
//	if (bGetDependenciesSuccess)
//	{
//		for (auto &DependItem : local_Dependencies)
//		{
//			FString LongDependentPackageName = DependItem.ToString();
//			FString BelongModuleName = UFlibAssetManageHelper::GetAssetBelongModuleName(LongDependentPackageName);
//
//			// add a new asset to module category
//			auto AddNewAssetItemLambda = [&InAssetRegistryModule,&OutDependencies](
//				FAssetDependenciesDetail& ModuleAssetDependDetail, 
//				FString AssetPackageName)
//			{
//				if (ModuleAssetDependDetail.mDependAsset.Find(AssetPackageName) == INDEX_NONE)
//				{
//					ModuleAssetDependDetail.mDependAsset.Add(AssetPackageName);
//					GatherAssetDependicesInfoRecursively(InAssetRegistryModule, AssetPackageName, OutDependencies);
//				}
//			};
//
//			if (OutDependencies.mDependencies.Contains(BelongModuleName))
//			{
//				// UE_LOG(LogTemp, Log, TEXT("Belong Module is %s,Asset is %s"), *BelongModuleName, *LongDependentPackageName);
//				FAssetDependenciesDetail* ModuleCategory=OutDependencies.mDependencies.Find(BelongModuleName);
//				AddNewAssetItemLambda(*ModuleCategory, LongDependentPackageName);
//			}
//			else 
//			{
//				// UE_LOG(LogTemp, Log, TEXT("New Belong Module is %s,Asset is %s"), *BelongModuleName,*LongDependentPackageName);
//				FAssetDependenciesDetail& NewModuleCategory = OutDependencies.mDependencies.Add(BelongModuleName, FAssetDependenciesDetail{});
//				NewModuleCategory.mModuleCategory = BelongModuleName;
//				AddNewAssetItemLambda(NewModuleCategory, LongDependentPackageName);
//			}
//		}
//	}
//}
//
//FString UFlibAssetManageHelper::ConvAssetPathFromRelativeToAbs(const FString& InProjectDir, const FString& InRelativePath)
//{
//#if !PLATFORM_WINDOWS
//	FString ProjectPath = UFlibAssetManageHelper::ConvPath_Slash2BackSlash(InProjectDir);
//#else
//	FString ProjectPath = UFlibAssetManageHelper::ConvPath_BackSlash2Slash(InProjectDir);
//#endif
//	FString Resault;
//	if (!FPaths::DirectoryExists(ProjectPath))
//		return Resault;
//
//	FString ModuleName;
//	FString AssetName;
//	{
//		TArray<FString> PathArray;
//		InRelativePath.ParseIntoArray(PathArray,TEXT("/"));
//		ModuleName = PathArray[0];
//		AssetName = PathArray[PathArray.Num()-1];
//	}
//	
//	{
//		FString ContentRelative = UKismetStringLibrary::GetSubstring(InRelativePath, ModuleName.Len()+1, InRelativePath.Len());
//		FString ContentAbsPath;
//		if (ModuleName == TEXT("Game"))
//			ContentAbsPath = FPaths::Combine(ProjectPath, TEXT("Content"), ContentRelative);
//		if (ModuleName == TEXT("Engine"))
//			ContentAbsPath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir()), ContentRelative);		
//		if (IPluginManager::Get().FindPlugin(ModuleName).IsValid())
//			ContentAbsPath= FPaths::Combine(FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin(ModuleName)->GetContentDir()), ContentRelative);
//
//		Resault = ContentAbsPath;
//	}
//
//	bool bIsDir = FPaths::DirectoryExists(Resault);
//
//	if (bIsDir)
//	{
//		return Resault;
//	}
//
//	
//	FString SearchDir = UKismetStringLibrary::GetSubstring(Resault, 0, Resault.Len()- AssetName.Len());
//
//	TArray<FString> localFindFiles;
//	IFileManager::Get().FindFiles(localFindFiles, *SearchDir, nullptr);
//
//	for (const auto& Item : localFindFiles)
//	{
//		if (Item.Contains(AssetName) && Item[AssetName.Len()] == '.')
//		{
//			Resault = FPaths::Combine(SearchDir, Item);
//			break;
//		}
//	}
//
//	Resault = UFlibAssetManageHelper::ConvPath_BackSlash2Slash(Resault);
//	bool bIsFile = FPaths::FileExists(Resault);
//	return bIsFile ? Resault : TEXT("");
//	
//}
//
//void UFlibAssetManageHelper::ConvAssetPathListFromRelativeToAbs(const FString& InProjectDir, const TArray<FString>& InRelativePathList, TArray<FString>& OutAbsPath)
//{
//	for (const auto& AssetItem : InRelativePathList)
//	{
//		FString AbsPathResault = UFlibAssetManageHelper::ConvAssetPathFromRelativeToAbs(InProjectDir, AssetItem);
//		OutAbsPath.Add(AbsPathResault);
//	}
//}
//
//FString UFlibAssetManageHelper::ConvAssetPathFromAbsToRelative(const FString& InProjectDir, const FString& InAbsPath)
//{
//#if !PLATFORM_WINDOWS
//	FString AssetAbsPath = UFlibAssetManageHelper::ConvPath_Slash2BackSlash(InAbsPath);
//#else
//	FString AssetAbsPath = UFlibAssetManageHelper::ConvPath_BackSlash2Slash(InAbsPath);
//#endif
//	FString Resault;
//	if (!(FPaths::DirectoryExists(InProjectDir) || FPaths::DirectoryExists(AssetAbsPath))/* || !AssetAbsPath.Contains(InProjectDir)*/) 
//		return Resault;
//
//	bool bIsDir = FPaths::DirectoryExists(AssetAbsPath);
//	bool bIsFile = FPaths::FileExists(AssetAbsPath);
//
//	{
//		FString AssetModuleName;
//		FString AssetModuleAbsPath;
//		FString AssetModuleRelativePath;
//
//		TArray<FString> BreakPath;
//		AssetAbsPath.ParseIntoArray(BreakPath, TEXT("/"));
//		int32 ContentIndex=-1;
//		if (BreakPath.Find(TEXT("Content"), ContentIndex))
//		{
//			int32 FindContentStartCount = 0;
//			if (ContentIndex != -1)
//			{
//				AssetModuleName = BreakPath[ContentIndex - 1];
//				FindContentStartCount += ContentIndex;
//			}
//			
//			for (int32 index = 0; index < ContentIndex; ++index)
//			{
//				FindContentStartCount+=BreakPath[index].Len();
//			}
//			AssetModuleAbsPath = UKismetStringLibrary::GetSubstring(AssetAbsPath, 0, FindContentStartCount-1);
//		}
//		
//
//		// /Game
//		if (AssetModuleAbsPath.EndsWith(AssetModuleName) && AssetAbsPath.Contains(AssetModuleAbsPath))
//		{
//			AssetModuleRelativePath = TEXT("/Game");
//		}
//		// Engine
//		if (AssetModuleName == TEXT("Engine") && AssetModuleAbsPath.EndsWith(TEXT("Engine")))
//		{
//			AssetModuleAbsPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
//			AssetModuleRelativePath = TEXT("/Engine");
//		}
//		// Other Plugin
//		{
//			FString PluginModulePath;
//			if (UFlibAssetManageHelper::GetPluginModuleAbsDir(AssetModuleName, PluginModulePath))
//			{
//				AssetModuleAbsPath = PluginModulePath;
//				AssetModuleRelativePath = TEXT("/") + AssetModuleName;
//			}
//		}
//
//		FString RelativePath = UKismetStringLibrary::GetSubstring(AssetAbsPath, AssetModuleAbsPath.Len() + FString(TEXT("/Content")).Len(), AssetAbsPath.Len() - AssetModuleAbsPath.Len());
//		if (bIsDir)
//		{
//			Resault = AssetModuleRelativePath / RelativePath;
//		}
//
//		if (bIsFile)
//		{
//			Resault = AssetModuleRelativePath / RelativePath;
//			int32 FindIndex;
//			if (Resault.FindLastChar('.', FindIndex))
//			{
//				Resault.RemoveAt(FindIndex, Resault.Len() - FindIndex);
//
//				int32 LastBackSlant;
//				Resault.FindLastChar('/', LastBackSlant);
//				Resault += TEXT(".") + UKismetStringLibrary::GetSubstring(Resault, LastBackSlant + 1, Resault.Len() - LastBackSlant);
//			}
//		}
//
//
//	}
//
//
//	return Resault;
//}
//
//void UFlibAssetManageHelper::ConvAssetPathListFromAbsToRelative(const FString& InProjectDir, const TArray<FString>& InAbsPathList, TArray<FString>& OutRelativePath)
//{
//	for (const auto& AssetItem : InAbsPathList)
//	{
//		FString RelativePathResault=UFlibAssetManageHelper::ConvAssetPathFromAbsToRelative(InProjectDir, AssetItem);
//		OutRelativePath.Add(RelativePathResault);
//	}
//}
//
//FString UFlibAssetManageHelper::ConvPath_Slash2BackSlash(const FString& InPath)
//{
//	FString ResaultPath;
//	TArray<FString> OutArray;
//	/*InPath.ParseIntoArray(OutArray, TEXT("\\"));
//	if (OutArray.Num() == 1 && OutArray[0] == InPath)
//	{
//		InPath.ParseIntoArray(OutArray, TEXT("/"));
//	}*/
//	InPath.ParseIntoArray(OutArray, TEXT("/"));
//	int32 OutArrayNum = OutArray.Num();
//	for (int32 Index = 0; Index < OutArrayNum; ++Index)
//	{
//		if (!OutArray[Index].IsEmpty() && Index < OutArrayNum - 1)/* && FPaths::DirectoryExists(ResaultPath + item)*/
//		{
//			ResaultPath.Append(OutArray[Index]);
//			ResaultPath.Append(TEXT("\\"));
//		}
//		else {
//			ResaultPath.Append(OutArray[Index]);
//		}
//	}
//	return ResaultPath;
//}
//
//FString UFlibAssetManageHelper::ConvPath_BackSlash2Slash(const FString& InPath)
//{
//	FString ResaultPath;
//	TArray<FString> OutArray;
//	InPath.ParseIntoArray(OutArray, TEXT("\\"));
//	if (OutArray.Num() == 1 && OutArray[0] == InPath)
//	{
//		InPath.ParseIntoArray(OutArray, TEXT("/"));
//	}
//	int32 OutArrayNum = OutArray.Num();
//	for(int32 Index=0; Index < OutArrayNum;++Index)
//	{
//		if (!OutArray[Index].IsEmpty() && Index < OutArrayNum -1 )/* && FPaths::DirectoryExists(ResaultPath + item)*/
//		{
//			ResaultPath.Append(OutArray[Index]);
//			ResaultPath.Append(TEXT("/"));
//		}
//		else {
//			ResaultPath.Append(OutArray[Index]);
//		}
//	}
//	return ResaultPath;
//}
//
//
//
//bool UFlibAssetManageHelper::GetAssetAbsPathListFormRelativeFolder(const FString& InProjectDir,const FString& InRelativeAssetPath, TArray<FString>& OutAssetList)
//{
//	FString local_AbsAssetPath=UFlibAssetManageHelper::ConvAssetPathFromRelativeToAbs(InProjectDir, InRelativeAssetPath);
//	if (!FPaths::DirectoryExists(local_AbsAssetPath))
//		return false;
//	TArray<FString> localFileList;
//	UFlibAssetManageHelper::FindFilesRecursive(local_AbsAssetPath, OutAssetList);
//
//	return true;
//}
//bool UFlibAssetManageHelper::FindFilesRecursive(const FString& InStartDir, TArray<FString>& OutFileList, bool InRecursive,bool InClearList)
//{
//	TArray<FString> CurrentFolderFileList;
//	if (!FPaths::DirectoryExists(InStartDir))
//		return false;
//
//
//	// IFileManager::Get().FindFiles(CurrentFolderFileList, *InStartDir, true, true);
//	FFillArrayDirectoryVisitor FileVisitor;
//	IFileManager::Get().IterateDirectoryRecursively(*InStartDir, FileVisitor);
//
//	OutFileList.Append(FileVisitor.Files);
//	
//	return true;
//}
//
//
//void UFlibAssetManageHelper::ExportProjectAssetDependencies(const FString& InProjectDir, FAssetDependenciesInfo& OutAllDependencies)
//{
//	TArray<FString> GameAssetAbsPathList;
//	TArray<FString> GameAssetRelativePathList;
//	UFlibAssetManageHelper::GetAssetAbsPathListFormRelativeFolder(InProjectDir, TEXT("/Game"), GameAssetAbsPathList);
//	UFlibAssetManageHelper::ConvAssetPathListFromAbsToRelative(InProjectDir, GameAssetAbsPathList, GameAssetRelativePathList);
//	UFlibAssetManageHelper::GetAssetListDependencies(GameAssetRelativePathList, OutAllDependencies);
//}
//
//
//FString UFlibAssetManageHelper::GetAssetBelongModuleName(const FString& InAssetRelativePath)
//{
//	FString LongDependentPackageName = InAssetRelativePath;
//
//	int32 BelongModuleNameEndIndex = LongDependentPackageName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1);
//	FString BelongModuleName = UKismetStringLibrary::GetSubstring(LongDependentPackageName, 1, BelongModuleNameEndIndex - 1);// (LongDependentPackageName, BelongModuleNameEndIndex);
//
//	return BelongModuleName;
//}
//
//void UFlibAssetManageHelper::GetAllInValidAssetInProject(const FString& InProjectDir, FAssetDependenciesInfo InAllDependencies, TArray<FString> &OutInValidAsset)
//{
//	TArray<FString> Keys;
//	InAllDependencies.mDependencies.GetKeys(Keys);
//	for (const auto& ModuleItem : Keys)
//	{
//		// ignore search /Script Asset
//		if (ModuleItem == TEXT("Script"))
//			continue;
//		FAssetDependenciesDetail ModuleDependencies = InAllDependencies.mDependencies[ModuleItem];
//
//		for (const auto& AssetItem : ModuleDependencies.mDependAsset)
//		{
//			FString AssetAbsPath = UFlibAssetManageHelper::ConvAssetPathFromRelativeToAbs(InProjectDir,AssetItem);
//			if (!FPaths::FileExists(AssetAbsPath))
//			{
//				OutInValidAsset.Add(AssetItem);
//			}
//		}
//		
//	}
//}
//bool UFlibAssetManageHelper::SerializeAssetDependenciesToJson(const FAssetDependenciesInfo& InAssetDependencies, FString& OutJsonStr)
//{
//	OutJsonStr.Empty();
//	TSharedPtr<FJsonObject> RootJsonObject = MakeShareable(new FJsonObject);
//	{
//		// collect all module name
//		TArray<FString> AssetCategoryList;
//		InAssetDependencies.mDependencies.GetKeys(AssetCategoryList);
//		{
//			TArray<TSharedPtr<FJsonValue>> JsonCategoryList;
//			for (const auto& AssetCategoryItem : AssetCategoryList)
//			{
//				JsonCategoryList.Add(MakeShareable(new FJsonValueString(AssetCategoryItem)));
//			}
//			RootJsonObject->SetArrayField(JSON_MODULE_LIST_SECTION_NAME, JsonCategoryList);
//		}
//		// collect all invalid asset
//		{
//			TArray<FString> AllInValidAssetList;
//			UFlibAssetManageHelper::GetAllInValidAssetInProject(UKismetSystemLibrary::GetProjectDirectory(), InAssetDependencies,AllInValidAssetList);
//			if (AllInValidAssetList.Num() > 0)
//			{
//				TArray<TSharedPtr<FJsonValue>> JsonInvalidAssetList;
//				for (const auto& InValidAssetItem : AllInValidAssetList)
//				{
//					JsonInvalidAssetList.Add(MakeShareable(new FJsonValueString(InValidAssetItem)));
//				}
//				RootJsonObject->SetArrayField(JSON_ALL_INVALID_ASSET_SECTION_NAME, JsonInvalidAssetList);
//			}
//		}
//
//		// serialize asset list
//		for (const auto& AssetCategoryItem : AssetCategoryList)
//		{
//			{
//				// TSharedPtr<FJsonObject> CategoryJsonObject = MakeShareable(new FJsonObject);
//
//				TArray<TSharedPtr<FJsonValue>> CategoryAssetListJsonEntity;
//				const FAssetDependenciesDetail& CategortItem = InAssetDependencies.mDependencies[AssetCategoryItem];
//				for (const auto& AssetItem : CategortItem.mDependAsset)
//				{
//					CategoryAssetListJsonEntity.Add(MakeShareable(new FJsonValueString(AssetItem)));
//				}
//				// CategoryJsonObject->SetArrayField(AssetCategoryItem, CategoryAssetListJsonEntity);
//
//				RootJsonObject->SetArrayField(AssetCategoryItem, CategoryAssetListJsonEntity);
//			}	
//		}
//	}
//	
//	auto JsonWriter = TJsonWriterFactory<>::Create(&OutJsonStr);
//	FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), JsonWriter);
//	return true;
//}
//
//
//bool UFlibAssetManageHelper::DeserializeAssetDependencies(const FString& InStream, FAssetDependenciesInfo& OutAssetDependencies)
//{
//	if (InStream.IsEmpty()) return false;
//
//	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InStream);
//	TSharedPtr<FJsonObject> JsonObject;;
//	if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
//	{
//		TArray<TSharedPtr<FJsonValue>> JsonModuleList = JsonObject->GetArrayField(JSON_MODULE_LIST_SECTION_NAME);
//		for (const auto& JsonModuleItem : JsonModuleList)
//		{
//			
//			FString ModuleName = JsonModuleItem->AsString();
//			
//			TArray<TSharedPtr<FJsonValue>> JsonAssetList = JsonObject->GetArrayField(ModuleName);
//			TArray<FString> AssetList;
//
//			for (const auto& JsonAssetItem : JsonAssetList)
//			{
//				FString AssetInfo=JsonAssetItem->AsString();
//				AssetList.Add(AssetInfo);
//			}
//			OutAssetDependencies.mDependencies.Add(ModuleName, FAssetDependenciesDetail{ ModuleName,AssetList });
//		}
//	}
//	return true;
//}
//
//bool UFlibAssetManageHelper::SaveStringToFile(const FString& InFile, const FString& InString)
//{
//	return FFileHelper::SaveStringToFile(InString, *InFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
//}
//
//bool UFlibAssetManageHelper::LoadFileToString(const FString& InFile, FString& OutString)
//{
//	return FFileHelper::LoadFileToString(OutString, *InFile);
//}
//
//bool UFlibAssetManageHelper::GetPluginModuleAbsDir(const FString& InPluginModuleName, FString& OutPath)
//{
//	bool bFindResault = false;
//	TSharedPtr<IPlugin> FoundModule = IPluginManager::Get().FindPlugin(InPluginModuleName);
//
//	if (FoundModule.IsValid())
//	{
//		bFindResault = true;
//		OutPath = FPaths::ConvertRelativePathToFull(FoundModule->GetBaseDir());
//	}
//	return bFindResault;
//}
//
//bool UFlibAssetManageHelper::GetEnableModuleAbsDir(const FString& InModuleName, FString& OutPath)
//{
//	if (InModuleName.Equals(TEXT("Engine")))
//	{
//		OutPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
//		return true;
//	}
//	if (InModuleName.Equals(TEXT("Game")))
//	{
//		OutPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
//		return true;
//	}
//	return UFlibAssetManageHelper::GetPluginModuleAbsDir(InModuleName, OutPath);
//
//}
//
//FAssetDependenciesInfo UFlibAssetManageHelper::CombineAssetDependencies(const FAssetDependenciesInfo& A, const FAssetDependenciesInfo& B)
//{
//	FAssetDependenciesInfo resault;
//
//	auto CombineLambda = [&resault](const FAssetDependenciesInfo& InDependencies)
//	{
//		TArray<FString> Keys;
//		InDependencies.mDependencies.GetKeys(Keys);
//		for (const auto& Key : Keys)
//		{
//			if (!resault.mDependencies.Contains(Key))
//			{
//				resault.mDependencies.Add(Key, *InDependencies.mDependencies.Find(Key));
//			}
//			else
//			{
//				TArray<FString>& ExistingAssetList = resault.mDependencies.Find(Key)->mDependAsset;
//				const TArray<FString>& PaddingAssetList = InDependencies.mDependencies.Find(Key)->mDependAsset;
//				for (const auto& PaddingItem : PaddingAssetList)
//				{
//					if (!ExistingAssetList.Contains(PaddingItem))
//					{
//						ExistingAssetList.Add(PaddingItem);
//					}
//				}
//			}
//		}
//	};
//
//	CombineLambda(A);
//	CombineLambda(B);
//
//	return resault;
//
//	
//}
//
//FAssetDependenciesInfo UFlibAssetManageHelper::ParserNewDependencysInfo(const FAssetDependenciesInfo& InNewVersion, const FAssetDependenciesInfo& InOldVersion)
//{
//	FAssetDependenciesInfo resault;
//
//	TArray<FString> NewAKeys;
//	InNewVersion.mDependencies.GetKeys(NewAKeys);
//	TArray<FString> OldBKeys;
//	InOldVersion.mDependencies.GetKeys(OldBKeys);
//
//	for (const auto& AKey : NewAKeys)
//	{
//		if (OldBKeys.Contains(AKey))
//		{
//			const TArray<FString>& NewADepend = InNewVersion.mDependencies.Find(AKey)->mDependAsset;
//			const TArray<FString>& OldBDepend = InOldVersion.mDependencies.Find(AKey)->mDependAsset;
//			for (const auto& ADependItem : NewADepend)
//			{
//				if (!OldBDepend.Contains(ADependItem))
//				{
//					if (!resault.mDependencies.Contains(AKey))
//						resault.mDependencies.Add(AKey, FAssetDependenciesDetail{ AKey,TArray<FString>{} });
//					resault.mDependencies.Find(AKey)->mDependAsset.Add(ADependItem);
//				}
//			}
//		}
//		else
//		{
//			resault.mDependencies.Add(AKey, *InNewVersion.mDependencies.Find(AKey));
//		}
//	}
//
//	return resault;
//}
//
//bool UFlibAssetManageHelper::ConvAssetRelativePathToCookedPath(const FString& InProjectAbsDir, const FString& InPlatformName, const FString& InRelativeAssetPath, TArray<FString>& OutCookedAssetPath, TArray<FString>& OutCookedAssetRelativePath)
//{
//	if (!FPaths::DirectoryExists(InProjectAbsDir) || !IsValidPlatform(InPlatformName))
//		return false;
//	
//	FString EngineAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
//	FString CookedRootDir = FPaths::Combine(InProjectAbsDir, TEXT("Saved/Cooked"),InPlatformName);
//	FString ProjectName = FApp::GetProjectName();
//	FString AssetAbsPath = UFlibAssetManageHelper::ConvAssetPathFromRelativeToAbs(InProjectAbsDir,InRelativeAssetPath);
//	
//	FString AssetModuleName=InRelativeAssetPath;
//	{
//		AssetModuleName.RemoveFromStart(TEXT("/"));
//		int32 secondSlashIndex=-1;
//		AssetModuleName.FindChar('/', secondSlashIndex);
//		AssetModuleName = UKismetStringLibrary::GetSubstring(AssetModuleName, 0, secondSlashIndex);
//	}
//
//	bool bIsEngineModule = false;
//	FString AssetBelongModuleBaseDir;
//	FString AssetCookedNotPostfixPath;
//	{
//		
//		if (UFlibAssetManageHelper::GetEnableModuleAbsDir(AssetModuleName, AssetBelongModuleBaseDir))
//		{
//			if (AssetBelongModuleBaseDir.Contains(EngineAbsDir))
//				bIsEngineModule = true;
//		}
//		
//		FString AssetCookedRelativePath;
//		if (bIsEngineModule)
//		{
//			AssetCookedRelativePath = TEXT("Engine") / UKismetStringLibrary::GetSubstring(AssetAbsPath, EngineAbsDir.Len()-1, AssetAbsPath.Len() - EngineAbsDir.Len());		
//		}
//		else
//		{
//			AssetCookedRelativePath = ProjectName / UKismetStringLibrary::GetSubstring(AssetAbsPath, InProjectAbsDir.Len()-1, AssetAbsPath.Len() - InProjectAbsDir.Len());
//		}
//		
//		// remove .uasset / .umap postfix
//		{
//			int32 lastDotIndex = 0;
//			AssetCookedRelativePath.FindLastChar('.', lastDotIndex);
//			AssetCookedRelativePath.RemoveAt(lastDotIndex, AssetCookedRelativePath.Len() - lastDotIndex);
//		}
//
//		AssetCookedNotPostfixPath = FPaths::Combine(CookedRootDir, AssetCookedRelativePath);
//	}
//
//
//
//	FFillArrayDirectoryVisitor FileVisitor;
//	FString SearchDir;
//	{
//		int32 lastSlashIndex;
//		AssetCookedNotPostfixPath.FindLastChar('/', lastSlashIndex);
//		SearchDir = UKismetStringLibrary::GetSubstring(AssetCookedNotPostfixPath, 0, lastSlashIndex);
//	}
//	IFileManager::Get().IterateDirectory(*SearchDir, FileVisitor);
//	for (const auto& FileItem : FileVisitor.Files)
//	{
//		if (FileItem.Contains(AssetCookedNotPostfixPath) && FileItem[AssetCookedNotPostfixPath.Len()] == '.')
//		{
//			OutCookedAssetPath.Add(FileItem);
//			{
//				FString AssetCookedRelativePath = UKismetStringLibrary::GetSubstring(FileItem, CookedRootDir.Len()+1, FileItem.Len() - CookedRootDir.Len());
//				OutCookedAssetRelativePath.Add(FPaths::Combine(TEXT("../../../"), AssetCookedRelativePath));
//			}
//		}
//	}
//	return true;
//}
//
//bool UFlibAssetManageHelper::IsValidPlatform(const FString& PlatformName)
//{
//
//	for (const auto& PlatformItem : UFlibAssetManageHelper::GetAllTargetPlatform())
//	{
//		if (PlatformItem.Equals(PlatformName))
//		{
//			return true;
//		}
//	}
//	return false;
//}
//
//TArray<FString> UFlibAssetManageHelper::GetAllTargetPlatform()
//{
//#ifdef __DEVELOPER_MODE__
//	TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();
//	TArray<FString> result;
//
//	for (const auto& PlatformItem : Platforms)
//	{
//		result.Add(PlatformItem->PlatformName());
//	}
//
//#else
//	TArray<FString> result = {
//		"AllDesktop",
//		"MacClient",
//		"MacNoEditor",
//		"MacServer",
//		"Mac",
//		"WindowsClient",
//		"WindowsNoEditor",
//		"WindowsServer",
//		"Windows",
//		"Android",
//		"Android_ASTC",
//		"Android_ATC",
//		"Android_DXT",
//		"Android_ETC1",
//		"Android_ETC1a",
//		"Android_ETC2",
//		"Android_PVRTC",
//		"AndroidClient",
//		"Android_ASTCClient",
//		"Android_ATCClient",
//		"Android_DXTClient",
//		"Android_ETC1Client",
//		"Android_ETC1aClient",
//		"Android_ETC2Client",
//		"Android_PVRTCClient",
//		"Android_Multi",
//		"Android_MultiClient",
//		"HTML5",
//		"IOSClient",
//		"IOS",
//		"TVOSClient",
//		"TVOS",
//		"LinuxClient",
//		"LinuxNoEditor",
//		"LinuxServer",
//		"Linux",
//		"Lumin",
//		"LuminClient" 
//	};
//
//#endif
//	return result;
//}
//
//bool UFlibAssetManageHelper::GetCookCommandFromAssetDependencies(const FString& InProjectDir,const FString& InPlatformName,const FAssetDependenciesInfo& InAssetDependencies,const TArray<FString> &InCookParams, TArray<FString>& OutCookCommand)
//{
//	if (!FPaths::DirectoryExists(InProjectDir) || !UFlibAssetManageHelper::IsValidPlatform(InPlatformName))
//		return false;
//	OutCookCommand.Empty();
//	// TArray<FString> resault;
//	TArray<FString> Keys;
//	InAssetDependencies.mDependencies.GetKeys(Keys);
//
//	for (const auto& Key : Keys)
//	{
//		if(Key.Equals(TEXT("Script")))
//			continue;
//		for (const auto& AssetItem : InAssetDependencies.mDependencies.Find(Key)->mDependAsset)
//		{
//			TArray<FString> CookedAssetAbsPath;
//			TArray<FString> CookedAssetRelativePath;
//			TArray<FString> FinalCookedCommand;
//			if (ConvAssetRelativePathToCookedPath(InProjectDir,InPlatformName,AssetItem,CookedAssetAbsPath, CookedAssetRelativePath))
//			{
//				if (CombineCookedAssetCommand(CookedAssetAbsPath, CookedAssetRelativePath, InCookParams, FinalCookedCommand))
//				{
//					OutCookCommand.Append(FinalCookedCommand);
//				}
//			}
//
//		}
//	}
//	return true;
//}
//
//bool UFlibAssetManageHelper::CombineCookedAssetCommand(const TArray<FString> &InAbsPath, const TArray<FString>& InRelativePath, const TArray<FString>& InParams, TArray<FString>& OutCommand)
//{
//	OutCommand.Empty();
//	if (InAbsPath.Num() != InRelativePath.Num())
//		return false;
//	int32 AssetNum = InAbsPath.Num();
//	for (int32 index = 0; index < AssetNum; ++index)
//	{
//		FString CurrentCommand = TEXT("\"")+InAbsPath[index] + TEXT("\" \"") + InRelativePath[index]+TEXT("\"");
//		for (const auto& Param : InParams)
//		{
//			CurrentCommand += TEXT(" ") + Param;
//		}
//		OutCommand.Add(CurrentCommand);
//	}
//	return true;
//}
//
//bool UFlibAssetManageHelper::ExportCookPakCommandToFile(const TArray<FString>& InCommand, const FString& InFile)
//{
//	return FFileHelper::SaveStringArrayToFile(InCommand, *InFile);
//}
//
//bool UFlibAssetManageHelper::RemovePackageAssetPostfix(const FString& InPackageName,FString& OutNoPostfixAsset)
//{
//	if (InPackageName.IsEmpty())
//		return false;
//	int32 FoundLastDot;
//	if (InPackageName.FindLastChar('.', FoundLastDot))
//	{
//		OutNoPostfixAsset = UKismetStringLibrary::GetSubstring(InPackageName, 0, FoundLastDot);
//	}
//	return true;
//}
//
//
//
