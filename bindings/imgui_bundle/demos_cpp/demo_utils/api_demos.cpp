#include "api_demos.h"
#include "imgui_md_wrapper.h"
#include "immapp/immapp.h"
#include "immapp/code_utils.h"
#include "immapp/snippets.h"
#include "demo_utils/subprocess.h"
#include "hello_imgui/internal/whereami/whereami_cpp.h"

#include <fplus/fplus.hpp>
#include <unordered_map>
#include <string>
#include <functional>
#include <filesystem>


std::string DemosAssetsFolder()
{
#ifndef __EMSCRIPTEN__
    return "demos_assets/";
#else
    return "/";
#endif
}


void ChdirBesideAssetsFolder()
{
    auto findDemoAssets = [](const std::filesystem::path& path) -> bool
    {
        if (std::filesystem::is_directory(path / DemosAssetsFolder()))
        {
            std::filesystem::current_path(path);
            if (! std::filesystem::is_directory(DemosAssetsFolder()))
                throw std::runtime_error("ChdirBesideAssetsFolder => fail setting path");
            HelloImGui::SetAssetsFolder(DemosAssetsFolder());
            return true;
        }
        else
            return false;
    };

    // 1. Try to find demo assets in current folder
    const auto& currentPath = std::filesystem::current_path();
    if ( findDemoAssets(currentPath) )
        return;

    // 2. Try to find demo assets in current folder parent
    if (findDemoAssets(currentPath.parent_path()))
        return;

    // 3. Try to find demo assets in exe folder
    std::filesystem::path exeFolder(wai_getExecutableFolder_string());
    if (findDemoAssets(exeFolder))
        return;

    // 3. Try to find demo assets in exe folder parent (for MSVC Debug/ and Release/ folders)
    if (findDemoAssets(exeFolder.parent_path()))
        return;

    std::cerr << "Could not find " << DemosAssetsFolder() << " folder!\n";
}



std::string MainPythonPackageFolder()
{
#ifdef __EMSCRIPTEN__
    return "/";
#else
    auto thisDir = std::filesystem::path(__FILE__).parent_path();
    auto grandParentDir = thisDir.parent_path().parent_path();
    return grandParentDir.string();
#endif
}

std::string DemoCppFolder()  { return MainPythonPackageFolder() + "/demos_cpp"; }
std::string DemoPythonFolder() { return MainPythonPackageFolder() + "/demos_python"; }

// memoized function
std::string ReadCode(const std::string& filename)
{
    static std::unordered_map<std::string, std::string> gFileContents;
    if (gFileContents.find(filename) != gFileContents.end())
        return gFileContents.at(filename);

    if (std::filesystem::exists(filename))
        gFileContents[filename] = fplus::read_text_file(filename)();
    else
        gFileContents[filename] = "";

    return gFileContents.at(filename);
}


std::string ReadCppCode(const std::string& demo_file_path)
{
    return ReadCode(DemoCppFolder() + "/" + demo_file_path + ".cpp");
}

std::string ReadPythonCode(const std::string& demo_file_path)
{
    return ReadCode(DemoPythonFolder() + "/" + demo_file_path + ".py");
}


void ShowPythonVsCppCode(const std::string& pythonCode, const std::string& cppCode, int nbLines)
{
    ImGui::PushID(pythonCode.c_str());

    const bool flagHalfWidth = !pythonCode.empty() && !cppCode.empty();

    Snippets::SnippetData snippetCpp, snippetPython;

    snippetCpp.Code = cppCode;
    snippetCpp.DisplayedFilename = "C++ code";
    snippetCpp.Language = Snippets::SnippetLanguage::Cpp;
    snippetCpp.HeightInLines = nbLines;

    snippetPython.Code = pythonCode;
    snippetPython.DisplayedFilename = "Python code";
    snippetPython.Language = Snippets::SnippetLanguage::Python;
    snippetPython.HeightInLines = nbLines;

    Snippets::ShowSideBySideSnippets(snippetCpp, snippetPython);
    ImGui::PopID();
}


void ShowPythonVsCppFile(const char* demo_file_path, int nbLines)
{
    std::string cpp_code = ReadCppCode(demo_file_path);
    std::string python_code = ReadPythonCode(demo_file_path);
    ShowPythonVsCppCode(python_code.c_str(), cpp_code.c_str(), nbLines);
}


#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

bool SpawnDemo(const std::string& demoName)
{
#ifndef __EMSCRIPTEN__
    std::string exeFolder = wai_getExecutableFolder_string();
    std::string exeFile = exeFolder + "/" + demoName;
#ifdef _WIN32
    exeFile += ".exe";
#endif
    if (std::filesystem::exists(exeFile))
    {
        const char *command_line[2] = {exeFile.c_str(), NULL};
        struct subprocess_s subprocess;
        subprocess_create(command_line, subprocess_option_no_window, &subprocess);
        return true;
    }
    else
        return false;
#else
    // This is for emscripten
    std::string jsCommandTemplate = R"(
        window.open("{demoName}.html", "hello", "width=900,height=600");
    )";
    std::string jsCommand = fplus::replace_tokens<std::string>("{demoName}", demoName, jsCommandTemplate);
    printf("%s\n", jsCommand.c_str());
    emscripten_run_script(jsCommand.c_str());
//    EM_ASM({
//      console.log('I received: ' + $0);
//    }, 100);
//    EM_ASM({
//        window.open($0, "hello", "width=900,height=600");
//        );}
    return true;
#endif
}


// [sub section]  BrowseToUrl()
// A platform specific utility to open an url in a browser
// (especially useful with emscripten version)
// Specific per platform includes for BrowseToUrl
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#elif defined(_WIN32)
#include <windows.h>
#include <Shellapi.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#endif

void BrowseToUrl(const char *url)
{
#if defined(__EMSCRIPTEN__)
    char js_command[1024];
    snprintf(js_command, 1024, "window.open(\"%s\");", url);
    emscripten_run_script(js_command);
#elif defined(_WIN32)
    ShellExecuteA( NULL, "open", url, NULL, NULL, SW_SHOWNORMAL );
#elif TARGET_OS_IPHONE
    // Nothing on iOS
#elif TARGET_OS_OSX
    char cmd[1024];
    snprintf(cmd, 1024, "open %s", url);
    system(cmd);
#elif defined(__linux__)
    char cmd[1024];
    snprintf(cmd, 1024, "xdg-open %s", url);
    int r = system(cmd);
    (void) r;
#endif
}

void BrowseToUrl(const std::string& url)
{
    BrowseToUrl(url.c_str());
}
