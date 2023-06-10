#compdef mace builder=mace
typeset -A opt_args
local context state line

trim() {
    local var="$*"
    # remove leading whitespace characters
    var="${var#"${var%%[![:space:]]*}"}"
    # remove trailing whitespace characters
    var="${var%"${var##*[![:space:]]}"}"
    printf '%s' "$var"
}

_parse-macefile-configs () {
  local input trimmed var val target dep TAB=$'\t' tmp IFS=
  VARIABLES=()
  while read input
  do
    trimmed="${input#"${input%%[![:space:]]*}"}"
    trimmed="${trimmed%"${trimmed##*[![:space:]]}"}"
    if [[ $input == *"MACE_ADD_CONFIG"* ]]; then
      # Skip if commented out
      if [[ $trimmed == "//"* ]] || [[ $trimmed == "/*"* ]]; then
      continue
      fi

      # check for MACE_ADD_CONFIG($name)
      [[ "$input" =~ '\(([^)]+)\)' ]] # regex, get str between ()
      val=${match##[ $TAB]#} # match gets put in $match variable
      VARIABLES[$match]=$val
    elif [[ $input == *"mace_add_config"* ]]; then
      # Skip if commented out
      if [[ $trimmed == "//"* ]] || [[ $trimmed == "/*"* ]]; then
      continue
      fi

      # check for mace_add_config(struct, $name)
      [[ "$input" =~ '\(&([^)]+), "([^)]+)"\)' ]] # regex, get str between ()
      val=${match[2]##[ $TAB]#} # match gets put in $match variable
      VARIABLES[${match[2]}]=$val
    fi
  done
}

_parse-macefile-targets () {
  local input trimmed var val target dep TAB=$'\t' tmp IFS=
  VARIABLES=()
  while read input
  do
    trimmed="${input#"${input%%[![:space:]]*}"}"
    trimmed="${trimmed%"${trimmed##*[![:space:]]}"}"
    if [[ $input == *"MACE_ADD_TARGET"* ]]; then
      # Skip if commented out
      if [[ $trimmed == "//"* ]] || [[ $trimmed == "/*"* ]]; then
      continue
      fi

      # check for MACE_ADD_TARGET($name)
      [[ "$input" =~ '\(([^)]+)\)' ]] # regex, get str between ()
      val=${match##[ $TAB]#} # match gets put in $match variable
      VARIABLES[$match]=$val
    elif [[ $input == *"mace_add_target"* ]]; then
      # Skip if commented out
      if [[ $trimmed == "//"* ]] || [[ $trimmed == "/*"* ]]; then
      continue
      fi

      # check for mace_add_target(struct, $name)
      [[ "$input" =~ '\(&([^)]+), "([^)]+)"\)' ]] # regex, get str between ()
      val=${match[2]##[ $TAB]#} # match gets put in $match variable
      VARIABLES[${match[2]}]=$val
    fi
  done
}

_mace() {
  local file
  local -i cdir=-1 ret=1  # integer
  local -A VARIABLES VAR_ARGS opt_args # associative array
  local basedir nul=$'\0'

  # VAR=VAL on the current command line
  for tmp in $words; do
    if [[ $tmp == (#b)([[:alnum:]_]##)=(*) ]]; then
      VAR_ARGS[${tmp[$mbegin[1],$mend[1]]}]=${(e)tmp[$mbegin[2],$mend[2]]}
    fi
  done
  keys=( ${(k)VAR_ARGS} ) # to be used in 'compadd -F keys'

  # Setting $state from current arguments
  _arguments '(-B --always-make)'{-B,--always-make}'[Build all targets without condition]'\
    '*'{-C,--directory=}'[Move to directory before anything else]:change to directory:->cdir'\
    '(-c --cc)'{-c,--cc=}'[Override C compiler]:macefile:->compiler'\
    '--debug=-[Print debug info]:debug options:->debug'\
    '(-g --config)'{-g,--config=}'[Name of config]:macefile:->config'\
    '(-f --file)'{-f,--file=}'[Specify input macefile. Defaults to macefile.c]:macefile:->file'\
    '(- *)'{-h,--help}'[Display help and exit]'\
    '(-j --jobs)'{-j+,--jobs=}'[Allow N jobs at once]:: : _guard "[0-9]#" "number of jobs"'\
    '(-n --dry-run)'{-n,--dry-run}"[Don't build, just echo commands]"\
    '(-s --silent)'{-s,--silent}"[Don't echo commands]"\
    '(- *)'{-v,--version}'[Display version and exit]'\
    '*:mace target:->target' && ret=0
  
  [[ $state = cdir ]] && cdir=-2 # What does this line do?

  # Getting current directory/$basedir
  basedir=${(j./.)${${~"${(@s.:.):-$PWD:${(Q)${opt_args[-C]:-$opt_args[--directory]}//\\:/$nul}}"}[(R)/*,cdir]}//$nul/:}
  VAR_ARGS[CURDIR]="${basedir}"

  # Change suggested options as a function of state
  case $state in
    (*dir)
    # Suggest directories
    _description directories expl "$state_descr"
    _files "$expl[@]" -W $basedir -/ && ret=0
    ;;
    
    (compiler)
    # Suggest compilers
    local -a compilers topics
    compilers=('tcc:tcc compiler' 'clang:LLVM compiler' 'gcc:GNU compiler')
    _describe 'command' compilers
    ;;

    (file)
    # Suggest files
    _description files expl "$state_descr"
    _files "$expl[@]" -W $basedir && ret=0
    ;;
    
    (config)
    # Suggest configs
    # Getting macefile from opt_args
    file=${(v)opt_args[(I)(-f|--file)]}

    # Check if there is a macefile in current dir
    if [[ -n $file ]]
    then
      [[ $file == [^/]* ]] && file=$basedir/$file
      [[ -r $file ]] || file=
    else
      if [[ -r $basedir/macefile.c ]]
      then
        file=$basedir/macefile.c
      elif [[ -r $basedir/Macefile.c ]]
      then
        file=$basedir/Macefile.c
      else
        file=''
      fi
    fi

    if [[ -n "$file" ]] # if $file is not empty
    then
      if zstyle -t ":completion:${curcontext}:configs" call-command; then
        # why ?
        _parse-macefile-configs < <(_call_program configs "$words[1]" -nsp --no-print-directory -f "$file" --always-make 2> /dev/null)
      else
        # why ?
        _parse-macefile-configs < $file
      fi
    fi
    _alternative \
      'configs:make config:compadd -Q -a CONFIGS' \
      'variables:make variable:compadd -F keys -k VARIABLES' \
      && ret=0
    ;;

    (target)
    # Suggest targets
    # Getting macefile from opt_args
    file=${(v)opt_args[(I)(-f|--file)]}

    # Check if there is a macefile in current dir
    if [[ -n $file ]]
    then
      [[ $file == [^/]* ]] && file=$basedir/$file
      [[ -r $file ]] || file=
    else
      if [[ -r $basedir/macefile.c ]]
      then
        file=$basedir/macefile.c
      elif [[ -r $basedir/Macefile.c ]]
      then
        file=$basedir/Macefile.c
      else
        file=''
      fi
    fi

    if [[ -n "$file" ]] # if $file is not empty
    then
      if zstyle -t ":completion:${curcontext}:targets" call-command; then
        # why ?
        _parse-macefile-targets < <(_call_program targets "$words[1]" -nsp --no-print-directory -f "$file" --always-make 2> /dev/null)
      else
        # why ?
        _parse-macefile-targets < $file
      fi
    fi

    _alternative \
      'targets:make target:compadd -Q -a TARGETS' \
      'variables:make variable:compadd -F keys -k VARIABLES' \
      && ret=0
  esac

  return ret
}

_mace "$@"
