#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/project/project.h"
#include "halley/tools/file/filesystem.h"
#include <set>

using namespace Halley;

Project::Project(const String& platform, Path projectRootPath, Path halleyRootPath)
	: platform(platform)
	, rootPath(projectRootPath)
	, halleyRootPath(halleyRootPath)
{
	importAssetsDatabase = std::make_unique<ImportAssetsDatabase>(getAssetsPath(), getAssetsPath() / "import.db");
	codegenDatabase = std::make_unique<ImportAssetsDatabase>(getGenPath(), getGenPath() / "import.db");

	initialisePlugins();
	assetImporter = std::make_unique<AssetImporter>(*this, std::vector<Path>{getSharedAssetsSrcPath(), getAssetsSrcPath()});
}

Project::~Project() = default;

Path Project::getAssetsPath() const
{
	return rootPath / "bin" / "assets";
}

Path Project::getAssetsSrcPath() const
{
	return rootPath / "assets_src";
}

Path Project::getSharedAssetsSrcPath() const
{
	return halleyRootPath / "shared_assets";
}

Path Project::getGenPath() const
{
	return rootPath / "gen";
}

Path Project::getGenSrcPath() const
{
	return rootPath / "gen_src";
}

ImportAssetsDatabase& Project::getImportAssetsDatabase()
{
	return *importAssetsDatabase;
}

ImportAssetsDatabase& Project::getCodegenDatabase()
{
	return *codegenDatabase;
}

const AssetImporter& Project::getAssetImporter() const
{
	return *assetImporter;
}

std::unique_ptr<IAssetImporter> Project::getAssetImporterOverride(AssetType type) const
{
	for (auto& plugin: plugins) {
		auto importer = plugin->getAssetImporter(type);
		if (importer) {
			return importer;
		}
	}
	return {};
}

void Project::initialisePlugins()
{
	auto pluginPath = halleyRootPath / "plugins";
	auto files = FileSystem::enumerateDirectory(pluginPath);
	for (auto& file: files) {
		// HACK: fix extension for OSX/Linux
		if (file.getExtension() == ".dll") {
			loadPlugin(file);
		}
	}

	std::set<String> knownPlatforms = { "pc" };
	for (auto& p: plugins) {
		auto platforms = p->getSupportedPlatforms();
		for (auto& plat: platforms) {
			if (plat != "*" && knownPlatforms.find(plat) == knownPlatforms.end()) {
				knownPlatforms.insert(plat);
			}
		}
	}

	if (knownPlatforms.find(platform) == knownPlatforms.end()) {
		throw Exception("Unknown platform: " + platform);
	}
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

Project::HalleyPluginPtr Project::loadPlugin(const Path& path)
{
	// HACK: abstract this/support OSX/support Linux
	auto module = LoadLibrary(path.getString().c_str());
	if (!module) {
		return {};
	}

	using CreateHalleyPluginFn = IHalleyPlugin*();
	using DestroyHalleyPluginFn = void(IHalleyPlugin*);

	auto createHalleyPlugin = reinterpret_cast<CreateHalleyPluginFn*>(GetProcAddress(module, "createHalleyPlugin"));
	auto destroyHalleyPlugin = reinterpret_cast<DestroyHalleyPluginFn*>(GetProcAddress(module, "destroyHalleyPlugin"));
	if (!createHalleyPlugin || !destroyHalleyPlugin) {
		FreeLibrary(module);
		return {};
	}

	return HalleyPluginPtr(createHalleyPlugin(), [=] (IHalleyPlugin* plugin)
	{
		destroyHalleyPlugin(plugin);
		FreeLibrary(module);
	});
}
