rem
rem  reformat rtklib .pos file so it plots with easyplot 
rem
findstr /B 2 pip.pos >pip1
gawk "{print $2,$3,$4,$5}" pip1 > pip2
gawk "gsub(/:/,""" """) $0" pip2 > pip3
gawk "{print ($1*3600+$2*60+$3),$4,$5,$6}" pip3 >pip.out
del pip1 pip2 pip3
