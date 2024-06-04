cmd_/home/zzzccc/Desktop/sucll-main/scull.mod := printf '%s\n'   scull.o | awk '!x[$$0]++ { print("/home/zzzccc/Desktop/sucll-main/"$$0) }' > /home/zzzccc/Desktop/sucll-main/scull.mod
