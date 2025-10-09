# Eureka Doom Editor

This is a map editor for idtech1 games (Doom, Doom 2, Heretic, Hexen, Strife and derivatives).
It is cross platform for desktop, thanks to its FLTK framework use.

## Coding style

Header files have the `h` extension, source files for the main app have `cc`, source files for testing have `cpp`.

Use tabs for indentation, including when aligning code. Consider a full tab to be 4 spaces, not eight.
Braces are Allman style. Local variables are camelCase. New types of any kind will be PascalCase.
New functions of any kind will be camelCase. Member variables will be mPrefixed. Global non-static
variables are gPrefixed. Staric ones, both for class and translation unit, are sPrefixed. Konstants
are kPrefixed. Static const items are skPrefixed.

Do not restyle existing code. Only apply the style for new code. Try not to modify imported code downloaded from
well-known libraries, unless explicitly instructed to.

Line length is 120 columns. Second function lines have their args left aligned to the opening
parentheses. Do NOT ever put single statements on the same lines as the control flow statement,
always put them on next line, indented. Single statements without braces are permitted, I am not so
foolish to fall for the "goto fail" bug. Obviously, put them around if/else blocks to reduce the
risk of dangling else, if only to silence the Clang compiler.

Do not put stupid useless parentheses in operations, except for && between ||, and when the shifters
are combined with additions, because of clang compiler complaints.

For classes, only have inline methods if they're single-line. Otherwise, put their definitions separately
(unless they're templates), even if the classes are local to source files. Order access by public, then protected, then
private. For each access area, start with types, then with methods, then with variables. Methods should be in this
order: constructor, copy constructor, copy assignment operator, move constructor, move assignment operator. When in
doubt, delete the copy constructor and copy assignment operator. Then: other constructors, then instance methods, then
static methods. Member data should first be static, then non-static. All non-static member initialization should
preferably be inside the class definition, only do it in the constructors (preferably in the initialization list) if it
depends on parameters.

Inside source files, the functions should be defined in the same order as they're declared in the header files. Overall
order is as such: inclusions, preprocessor macros, enum constants, structs and classes, global variables, local class
method implementations, static functions (use the `static` keyword, do not use empty namespaces), then global functions.

Avoid using `auto` for type inference, unless it's visible via casting or instantiation.

Prefer early `return`, `break` or `continue` whenever possible, instead of nesting and indenting too many control flow statements.

All files must have the GNU GPL copyright header. For new files, only add my name and the current year
for the copyright.

## Building
Always use cmake. If you want to unit test, the binary is test_general.

## Unit tests
Prefer using the Google Test ASSERT_* macros to the EXPECT_* ones, unless the current function is non-`void`.

## Doom editing guidelines
Coordinate axes are so: x to the right, y to the top. Most typical is for coordinates to be snapped to a grid of 8.
A usual sector height is 128, but of course it can vary by multiples of 8. Player size is 40x40x56.
Maximum walkable step height is 24.

### Eureka-specific behavior
Two-sided linedefs in Eureka always have both upper and lower textures set, even if the texture would not be visible in-game
due to sector heights. This is an editor convenience feature.

GRAY1 is a default texture that is automatically set for new sidedefs if they don't have any texture reference.

## Documentation repository

An accompanying repository with the documentation is at https://github.com/wesleywerner/eureka-docs.git, where I have write access. It contains ReStructured Text format documentation, which gets turned by an automatic script into a website when git-tagged.