# Correctness-only: observe completed inactive lists before they are published.
# Set up only after first-room capture; never use these stops as timing evidence.
define install-copper-palette
set $paletteFrames=0
break pop2CopperReady
commands
 silent
 set $paletteFrames=$paletteFrames+1
 set $buffer=display.front_^1
 set $base=display.pixels_[$buffer]
 set $even=display.copper_+$buffer*2*559
 set $odd=$even+559
 printf "COPPER_FRAME %lu base=%08lx crop=%d,%d\n",$paletteFrames,$base,display.viewport_.top,display.viewport_.left
 eval "dump binary memory @RUN@/copper-%d-even.bin $even $even+559",$paletteFrames
 eval "dump binary memory @RUN@/copper-%d-odd.bin $odd $odd+559",$paletteFrames
 set $colors=*(unsigned long*)g_integrationColors
 eval "dump binary memory @RUN@/copper-%d-colors.bin $colors $colors+2056",$paletteFrames
 set $transfer='NativeDisplay::(anonymous namespace)::transfer'
 eval "dump binary memory @RUN@/copper-%d-transfer.bin $transfer $transfer+256",$paletteFrames
 continue
end
end
