break *pop2CoreStopped
break *pop2CoreSegmentLoaded if g_coreLoadedSegment == 15
continue
if $pc != (unsigned long)&pop2CoreSegmentLoaded
 printf "RELEASE STARTUP FAILED: %s\n",g_coreReason
 quit 1
end
set $checkpoint=g_coreSegments[15][0]+0x416
delete 2
break *$checkpoint if *(unsigned short*)($a6+8)==0x6978 && *(unsigned short*)($a6+10)==1
continue
if $pc != $checkpoint
 printf "RELEASE STARTUP FAILED: %s\n",g_coreReason
 quit 1
end
printf "RELEASE OPENING COMPLETE segment=15 offset=0416 text=6978 page=1\n"
dump binary memory @RUN@/release-screen.bin g_integrationScreen g_integrationScreen+640*480
set $colors=*(unsigned long*)g_integrationColors
dump binary memory @RUN@/release-colors.bin $colors $colors+2056
detach
quit 0
