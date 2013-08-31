syntax on
set ts=2
set hlsearch
set expandtab

"" tagbar
nmap <F8> :TagbarToggle<CR> 

"" ctags 
"au BufWritePost *.c,*.cpp,*.h silent! !ctags -R &
"map <C-\> :tab split<CR>:exec("tag ".expand("<cword>"))<CR>
map <A-]> :vsp <CR>:exec("tag ".expand("<cword>"))<CR>

"" cscope




"" clip 
if has("unix")
    let os = substitute(system('uname'), "\n", "", "")
    if os == "Linux"
        vmap <C-c> y:call system("xclip -i -selection clipboard", getreg("\""))<CR>:call system("xclip -i", getreg("\""))<CR>
    elseif os == "Darwin"
        vmap <C-c> y:call system("pbcopy", getreg("\""))<CR>
    end
end

"" Power Line
""set rtp+=/home/zork/Public/PowerLine/powerline/bindings/vim
"set laststatus=2
"set noshowmode


