" Vim syntax file
" Language:     Eureka Definition (.ugh)
" First Author: Andrew Apted
" Last Change:  2018 Jun 18

" quit when a syntax file was already loaded
if exists("b:current_syntax")
    finish
endif

syn case ignore

syn match ughComment /#.*$/

syn keyword ughDirective include if else endif is
syn keyword ughDirective set clear

syn keyword ughSetting map_formats supported_games variant_of
syn keyword ughSetting player_size sky_color sky_flat
syn keyword ughSetting default_textures default_thing
syn keyword ughSetting feature

syn keyword ughEntry line linegroup special color
syn keyword ughEntry sector thing thinggroup
syn keyword ughEntry texture flat texturegroup
syn keyword ughEntry gen_line gen_field

syn keyword ughError spec_group default_port level_name

syn region ughString start=/"/ skip=/\\[\\"]/ end=/"/

syn match ughNumber "\<-\?\d\+\>"

syn keyword ughTodo containedin=ughComment TODO FIXME OPTIMISE OPTIMIZE WISH

hi def link ughComment    Comment
hi def link ughConstant   Constant
hi def link ughDirective  Special
hi def link ughEntry      Function
hi def link ughError      Error
hi def link ughNumber     Number
hi def link ughSetting    Statement
hi def link ughString     String
hi def link ughTodo       Todo

let b:current_syntax = "ugh"

" vim:ts=8
