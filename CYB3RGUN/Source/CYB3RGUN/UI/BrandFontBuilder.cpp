// CYB3RGUN THEGAME. Editor console command that turns the brand's font files into font face and font assets (D-081).
// The assets stay normal assets afterwards; the command only saves retyping six imports and four composite fonts.

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogBrandFontBuilder, Log, All);

namespace BrandFontBuilder
{
	struct FFaceSpec
	{
		const TCHAR* Font;
		const TCHAR* Typeface;
		const TCHAR* FileName;
	};

	/** The faces the website ships (D-081): the file each comes from and the typeface it becomes inside its font */
	const FFaceSpec Faces[] = {
		{ TEXT("Michroma"), TEXT("Regular"), TEXT("Michroma-Regular.ttf") },
		{ TEXT("SairaCondensed"), TEXT("Medium"), TEXT("SairaCondensed-Medium.ttf") },
		{ TEXT("SairaCondensed"), TEXT("Bold"), TEXT("SairaCondensed-Bold.ttf") },
		{ TEXT("ShareTechMono"), TEXT("Regular"), TEXT("ShareTechMono-Regular.ttf") },
		{ TEXT("SourceSans3"), TEXT("Regular"), TEXT("SourceSans3-Regular.ttf") },
		{ TEXT("SourceSans3"), TEXT("Semibold"), TEXT("SourceSans3-Semibold.ttf") },
	};

	const TCHAR* Folder = TEXT("/Game/CYB3RGUN/UI/Fonts");

	bool Save(UObject* Asset)
	{
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Asset);
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset, *File, Args);
	}

	/** The first file in the folder whose name ends with the wanted name, so prefixed downloads are found too */
	FString FindFile(const FString& Directory, const FString& FileName)
	{
		TArray<FString> Found;
		IFileManager::Get().FindFiles(Found, *FPaths::Combine(Directory, TEXT("*.ttf")), true, false);
		for (const FString& Candidate : Found)
		{
			if (Candidate.EndsWith(FileName))
			{
				return FPaths::Combine(Directory, Candidate);
			}
		}
		return FString();
	}

	void Build(const TArray<FString>& Args)
	{
		if (Args.IsEmpty())
		{
			UE_LOG(LogBrandFontBuilder, Warning, TEXT("Brand.BuildFonts <folder with the ttf files>"));
			return;
		}
		const FString Directory = Args[0];

		TMap<FString, UFont*> Fonts;
		for (const FFaceSpec& Spec : Faces)
		{
			const FString Path = FindFile(Directory, Spec.FileName);
			TArray<uint8> Bytes;
			if (Path.IsEmpty() || !FFileHelper::LoadFileToArray(Bytes, *Path))
			{
				UE_LOG(LogBrandFontBuilder, Error, TEXT("No %s in %s"), Spec.FileName, *Directory);
				continue;
			}

			const FString FaceName = FString::Printf(TEXT("FF_%s_%s"), Spec.Font, Spec.Typeface);
			UPackage* FacePackage = CreatePackage(*FString::Printf(TEXT("%s/%s"), Folder, *FaceName));
			FacePackage->FullyLoad();
			UFontFace* Face = FindObject<UFontFace>(FacePackage, *FaceName);
			if (!Face)
			{
				Face = NewObject<UFontFace>(FacePackage, *FaceName, RF_Public | RF_Standalone | RF_Transactional);
			}
			Face->InitializeFromBulkData(Path, EFontHinting::Default, Bytes.GetData(), Bytes.Num());
			Face->LoadingPolicy = EFontLoadingPolicy::Inline;
			const bool bFaceSaved = Save(Face);

			const FString FontName = FString::Printf(TEXT("F_%s"), Spec.Font);
			UFont*& Font = Fonts.FindOrAdd(FontName);
			if (!Font)
			{
				UPackage* FontPackage = CreatePackage(*FString::Printf(TEXT("%s/%s"), Folder, *FontName));
				FontPackage->FullyLoad();
				Font = FindObject<UFont>(FontPackage, *FontName);
				if (!Font)
				{
					Font = NewObject<UFont>(FontPackage, *FontName, RF_Public | RF_Standalone | RF_Transactional);
				}
				Font->FontCacheType = EFontCacheType::Runtime;
				Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Reset();
			}
			FTypefaceEntry& Entry = Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
			Entry.Name = Spec.Typeface;
			Entry.Font = FFontData(Face);

			UE_LOG(LogBrandFontBuilder, Log, TEXT("%s from %s, %d bytes, saved %d"), *FaceName, *FPaths::GetCleanFilename(Path), Bytes.Num(), bFaceSaved ? 1 : 0);
		}

		for (const TPair<FString, UFont*>& Pair : Fonts)
		{
			UE_LOG(LogBrandFontBuilder, Log, TEXT("%s with %d typefaces, saved %d"), *Pair.Key, Pair.Value->GetInternalCompositeFont().DefaultTypeface.Fonts.Num(), Save(Pair.Value) ? 1 : 0);
		}
	}
}

static FAutoConsoleCommand GBrandBuildFontsCommand(
	TEXT("Brand.BuildFonts"),
	TEXT("Brand.BuildFonts <folder>. Creates or refreshes the brand font face and font assets in /Game/CYB3RGUN/UI/Fonts from the ttf files in the folder."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&BrandFontBuilder::Build));

#endif
