//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

// Cleans up all the pollution caused by Windows and Linux

#ifdef None
#undef None
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#ifdef grp1
//
//  Constant Declarations.
//

#undef ctlFirst    
#undef ctlLast     

//
//  Push buttons.
//
#undef psh1       
#undef psh2       
#undef psh3       
#undef psh4       
#undef psh5       
#undef psh6       
#undef psh7       
#undef psh8       
#undef psh9       
#undef psh10      
#undef psh11      
#undef psh12      
#undef psh13      
#undef psh14      
#undef psh15      
#undef pshHelp    
#undef psh16      

//
//  Checkboxes.
//
#undef chx1      
#undef chx2      
#undef chx3      
#undef chx4      
#undef chx5      
#undef chx6      
#undef chx7      
#undef chx8      
#undef chx9      
#undef chx10     
#undef chx11     
#undef chx12     
#undef chx13     
#undef chx14     
#undef chx15     
#undef chx16     

//
//  Radio buttons.
//
#undef rad1      
#undef rad2      
#undef rad3      
#undef rad4      
#undef rad5      
#undef rad6      
#undef rad7      
#undef rad8      
#undef rad9      
#undef rad10     
#undef rad11     
#undef rad12     
#undef rad13     
#undef rad14     
#undef rad15     
#undef rad16     

//
//  Groups, frames, rectangles, and icons.
//
#undef grp1    
#undef grp2    
#undef grp3    
#undef grp4    
#undef frm1    
#undef frm2    
#undef frm3    
#undef frm4    
#undef rct1    
#undef rct2    
#undef rct3    
#undef rct4    
#undef ico1    
#undef ico2    
#undef ico3    
#undef ico4    

//
//  Static text.
//
#undef stc1      
#undef stc2      
#undef stc3      
#undef stc4      
#undef stc5      
#undef stc6      
#undef stc7      
#undef stc8      
#undef stc9      
#undef stc10     
#undef stc11     
#undef stc12     
#undef stc13     
#undef stc14     
#undef stc15     
#undef stc16     
#undef stc17     
#undef stc18     
#undef stc19     
#undef stc20     
#undef stc21     
#undef stc22     
#undef stc23     
#undef stc24     
#undef stc25     
#undef stc26     
#undef stc27     
#undef stc28     
#undef stc29     
#undef stc30     
#undef stc31     
#undef stc32     

//
//  Listboxes.
//
#undef lst1     
#undef lst2     
#undef lst3     
#undef lst4     
#undef lst5     
#undef lst6     
#undef lst7     
#undef lst8     
#undef lst9     
#undef lst10    
#undef lst11    
#undef lst12    
#undef lst13    
#undef lst14    
#undef lst15    
#undef lst16    

//
//  Combo boxes.
//
#undef cmb1    
#undef cmb2    
#undef cmb3    
#undef cmb4    
#undef cmb5    
#undef cmb6    
#undef cmb7    
#undef cmb8    
#undef cmb9    
#undef cmb10   
#undef cmb11   
#undef cmb12   
#undef cmb13   
#undef cmb14   
#undef cmb15   
#undef cmb16   

//
//  Edit controls.
//
#undef edt1     
#undef edt2     
#undef edt3     
#undef edt4     
#undef edt5     
#undef edt6     
#undef edt7     
#undef edt8     
#undef edt9     
#undef edt10    
#undef edt11    
#undef edt12    
#undef edt13    
#undef edt14    
#undef edt15    
#undef edt16    

//
//  Scroll bars.
//
#undef scr1      
#undef scr2      
#undef scr3      
#undef scr4      
#undef scr5      
#undef scr6      
#undef scr7      
#undef scr8      

//
//  Controls
//
#undef ctl1      

#undef FILEOPENORD           
#undef MULTIFILEOPENORD      
#undef PRINTDLGORD           
#undef PRNSETUPDLGORD        
#undef FINDDLGORD            
#undef REPLACEDLGORD         
#undef FONTDLGORD            
#undef FORMATDLGORD31        
#undef FORMATDLGORD30        
#undef RUNDLGORD             

#if (WINVER >= 0x400)
#undef PAGESETUPDLGORD       
#undef NEWFILEOPENORD        
#undef PRINTDLGEXORD         
#undef PAGESETUPDLGORDMOTIF  
#undef COLORMGMTDLGORD       
#undef NEWFILEOPENV2ORD      
#endif /* WINVER >= 0x400) */

// 1581 - 1590
#if (NTDDI_VERSION >= NTDDI_VISTA)
#undef NEWFILEOPENV3ORD       
#endif // NTDDI_VISTA

// 1591 - 1600
#if (NTDDI_VERSION >= NTDDI_WIN7)
#undef NEWFORMATDLGWITHLINK   
#undef IDC_MANAGE_LINK        
#endif

#endif	// defined(grp1)
