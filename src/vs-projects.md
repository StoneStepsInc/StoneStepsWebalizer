# Visual Studio Projects

All settings that are shared between multiple projects in this solution are configured 
in property sheets that are attached to the relevant projects in various combinations. 

IMPORTANT: Do not change any settings at the project level, unless these settings apply 
to a particular platform and build configuration.

## webalizer

The `webalizer` project contains all C++ source files and references to the 3rd-party 
code required to build `webalizer.exe` for all platforms. This project uses property 
sheets described below and listed in the order of their precedence, from lower to 
higher.

### weblizer.props

This property sheet contains common settings for all platforms and configurations, 
such as the compiler warning level or the use of C++ exceptions and also defines 
base user macros used throughout the project.

### webalizer_win32.props and webalizer_x64.props

These property sheets contain settings and user macros specific to the platform 
identifier in their name, which are shared by all configurations built for the 
same platform in the project (e.g. Debug/Win32 and Release/Win32 share settings 
in webalizer_win32.props).

### webalizer_debug.props and webalizer_release.props

These property sheets contain settings and user macros specific to the configuration 
identifier in their name, which are shared by all platforms built for the same 
configuration in the project (e.g. `Debug/Win32` and `Debug/x64` share settings in 
`webalizer_debug.props`).

## package

This Utility-type project copies all distributable files into the project output directory.
The `package` project uses a custom build step script in `package.props` to 
copy all package files to their destinations. Note that even though the `Utility` type project
doesn't list `Custom Build Step` in the solution properties, it still runs the custom build 
step script in package.props attached to the build configuration.

The main reason for using the custom build step and not the custom built tool for each file, 
is that Visual Studio doesn't provide a generic way to supply an additional per-project-item 
qualifier that could be used to derive a subdirectory for each build item under the project 
output directory. One good way of doing this would be if there was a macro that would expand 
to the solution folder for each item, which could then define the structure of the final 
package.

The `package` project is set up to use a phony target called `phony-package-target` that is never
up to date, which forces Visual Studio to build the `package` project every time, regardless
whether the command was `Build` or `Rebuild`. This makes it easier to maintain the list of files,
without having to list each output file in the target list.

This project also includes all `webalizer*.props` property sheets to allow its scripts make 
references to the combined user-defined macros in these property sheets.

The `package` project is configured to build only for release configurations.

## test

A unit test project. Currently includes object files from the webalizer project build for
linking purposes.
