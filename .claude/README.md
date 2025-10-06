# Eureka Doom Editor

This is a map editor for idtech1 games (Doom, Doom 2, Heretic, Hexen, Strife and derivatives).
It is cross platform for desktop, thanks to its FLTK framework use.

## Coding style

Use tabs for indentation, including when aligning code. Consider a full tab to be 4 spaces, not eight.
Braces are Allman style. Local variables are camelCase. New types of any kind will be PascalCase.
New functions of any kind will be camelCase. Member variables will be mPrefixed. Global non-static
variables are gPrefixed. Staric ones, both for class and translation unit, are sPrefixed. Konstants
are kPrefixed. Static const items are skPrefixed.

Do not restyle existing code. Only apply the style for new code.

Line length is 120 columns. Second function lines have their args left aligned to the opening
parentheses. Do NOT ever put single statements on the same lines as the control flow statement,
always put them on next line, indented. Single statements without braces are permitted, I am not so
foolish to fall for the "goto fail" bug. Obviously, put them around if/else blocks to reduce the 
risk of dangling else, if only to silence the Clang compiler.

Do not put stupid useless parentheses in operations, except for && between ||, and when the shifters
are combined with additions, because of clang compiler complaints.

All files must have the GNU GPL copyright header. For new files, only add my name and the current year
for the copyright.

## Building
Always use cmake. If you want to unit test, the binary is test_general.
