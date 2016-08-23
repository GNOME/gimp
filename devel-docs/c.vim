" GIMP coding style for vim "

" To enable these vim rules for GIMP only, add this command to your vimrc:
" autocmd BufNewFile,BufRead /path/to/gimp/*.[ch] source /path/to/gimp/devel-docs/c.vim
"
" Do not use `set exrc` which is a security risk for your system since vim may
" be tricked into running shell commands by .vimrc files hidden in malicious
" projects (`set secure` won't protect you since it is not taken into account
" if the files are owned by you).

" GNU style
setlocal cindent
setlocal cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1
setlocal shiftwidth=2
setlocal softtabstop=2
setlocal textwidth=79
setlocal fo-=ro fo+=cql

" Tabs are always inserted as spaces.
set expandtab
" But if there are tabs already, show them as 8 columns.
setlocal tabstop=8

" Highlight in red trailing whitespaces and tabs everywhere.
highlight TrailingWhitespace ctermbg=LightRed guibg=LightRed
match TrailingWhitespace /\s\+$/
highlight ForbiddenTabs ctermbg=DarkRed guibg=DarkRed
2match ForbiddenTabs /\t/
