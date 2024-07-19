complete --command desktop-controller --no-files --arguments "(desktop-controller --list | tr ':' '\t')"

complete --command desktop-controller --short-option h --long-option help    --description 'Print help'
complete --command desktop-controller --short-option v --long-option version --description 'Print version'
complete --command desktop-controller --short-option l --long-option list    --description 'list all available controllers and exit'
