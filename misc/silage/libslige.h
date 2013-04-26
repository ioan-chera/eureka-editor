/*

   SLIGE - a random-level generator for DOOM
           by David M. Chess
           chess@theogeny.com      http://www.davidchess.com/
                                   http://users.aol.com/dmchess/
           chess@us.ibm.com        http://www.research.ibm.com/people/c/chess/

           http://www.doomworld.com/slige/

   August, 2000

   This source code is made available to the world in general,
   with the following requests:

   - If you're doing a port and you find that something or other needs
     twiddling to get it to compile, let me know and I'll stick in some
     ifdefs.  The current code assumes that Microsoft C has a horrible
     rand() function and uses str(n)icmp() instead of str(n)casecmp(),
     whereas everyone else has an ok rand() and str(n)casecmp().  And
     make sure that you compile with tight structure-packing (/Zp1 or
     -fpack_struct, I think) or DOOM won't understand the files.  Also,
     if you want to port to a non-Intel-byte-order platform, you've got
     a bit of work, but I think it's mostly isolated to DumpLevel,
     CloseDump, dump_texture_lmp, and a few others.  Good luck!  *8)

   - The current version should compile without any special switches
     except structure-packing, under both MSVC++ and DJGPP, without
     any warnings (unless you use -O3 in DJGPP, in which case you'll
     get some warnings that don't matter).  It should also compile
     and work under Linux gcc, but I've never tried it myself; if you
     do, write and tell me about it!  It should also compile for the
     RISC OS with the usual C compiler, thanks to Justin Fletcher.
     Anyone wanna do a Mac port?  I'd be glad to give advice...

   - If you make various changes and improvements to it and release
     a modified version yourself:

     - Write me and tell me about it so I can be pleased,
     - Mention me and SLIGE in the docs somewhere, and
     - Please *don't* call your new program "SLIGE".  SLIGE is
       my program.  Call yours "BLIGE" or "EGGISLES" or "MUMFO"
       or something like that.

   - If you want to do lots of wonderful things to the program to
     make it better, and then send me the result to incorporate into
     my next version, I strongly suggest telling me your plans first.
     Otherwise you may hear nothing from me for weeks after sending me
     your source mods (I don't check my mail that often sometimes), and
     then get a disappointing "thanks, but I don't want to take the
     program that direction, I have other plans" sort of reply.  Small
     bug fixes, of course, are always welcome!  All source mods sent
     to me become the property of the world at large.

   Otherwise, enjoy the program, and let me know what you think of the code!

   DC

*/

#ifndef __LIBSLIGE_H__
#define __LIBSLIGE_H__

/* API functions */

const char * Slige_GetError(void);

bool Slige_LoadConfig(const char *base_name);
void Slige_SetOption(const char *name, const char *value);
bool Slige_GenerateWAD(const char *filename);

#endif  /* __LIBSLIGE_H__ */

