break *pop2UIOverlayShown
commands
 silent
 printf "UI OVERLAY id=%lu kind=%lu planes=%08lx parity=%u dma=%04x\n",g_uiOverlayCount,g_uiOverlay,g_displayPlanes,g_displayParity,*(unsigned short*)0xdff002
 if g_uiOverlayCount == 1
  dump binary memory @RUN@/ui-1-logical.bin g_uiOverlayPixels g_uiOverlayPixels+307200
  dump binary memory @RUN@/ui-1-planes.bin g_displayPlanes g_displayPlanes+196608
  dump binary memory @RUN@/ui-1-copper.bin g_displayCopper g_displayCopper+2236
 end
 if g_uiOverlayCount == 2
  dump binary memory @RUN@/ui-2-logical.bin g_uiOverlayPixels g_uiOverlayPixels+307200
  dump binary memory @RUN@/ui-2-planes.bin g_displayPlanes g_displayPlanes+196608
  dump binary memory @RUN@/ui-2-copper.bin g_displayCopper g_displayCopper+2236
 end
 if g_uiOverlayCount == 3
  dump binary memory @RUN@/ui-3-logical.bin g_uiOverlayPixels g_uiOverlayPixels+307200
  dump binary memory @RUN@/ui-3-planes.bin g_displayPlanes g_displayPlanes+196608
  dump binary memory @RUN@/ui-3-copper.bin g_displayCopper g_displayCopper+2236
 end
 if g_uiOverlayCount == 4
  dump binary memory @RUN@/ui-4-logical.bin g_uiOverlayPixels g_uiOverlayPixels+307200
  dump binary memory @RUN@/ui-4-planes.bin g_displayPlanes g_displayPlanes+196608
  dump binary memory @RUN@/ui-4-copper.bin g_displayCopper g_displayCopper+2236
 end
 if g_uiOverlayCount == 5
  dump binary memory @RUN@/ui-5-logical.bin g_uiOverlayPixels g_uiOverlayPixels+307200
  dump binary memory @RUN@/ui-5-planes.bin g_displayPlanes g_displayPlanes+196608
  dump binary memory @RUN@/ui-5-copper.bin g_displayCopper g_displayCopper+2236
 end
 if g_uiOverlayCount == 6
  dump binary memory @RUN@/ui-6-logical.bin g_uiOverlayPixels g_uiOverlayPixels+307200
  dump binary memory @RUN@/ui-6-planes.bin g_displayPlanes g_displayPlanes+196608
  dump binary memory @RUN@/ui-6-copper.bin g_displayCopper g_displayCopper+2236
 end
 if g_uiOverlayCount == 7
  dump binary memory @RUN@/ui-7-logical.bin g_uiOverlayPixels g_uiOverlayPixels+307200
  dump binary memory @RUN@/ui-7-planes.bin g_displayPlanes g_displayPlanes+196608
  dump binary memory @RUN@/ui-7-copper.bin g_displayCopper g_displayCopper+2236
 end
 continue
end
break *pop2UIFixtureComplete
break *pop2CoreStopped
continue
if $pc != (unsigned long)&pop2UIFixtureComplete
 printf "UI FONT width=%u height=%u flags=%u\n",GfxBase->DefaultFont->tf_XSize,GfxBase->DefaultFont->tf_YSize,GfxBase->DefaultFont->tf_Flags
 printf "UI WRONG STOP reason=%s trap=%04lx seg=%lu offset=%04lx count=%lu\n",g_coreReason,g_coreStopTrap,g_coreStopSegment,g_coreStopOffset,g_coreCount
 quit 1
end
set $i=0
while $i<31
 printf "UI RESULT index=%lu value=%08lx\n",$i,g_uiReport[$i]
 set $i=$i+1
end
dump binary memory @RUN@/ui-keymaps.bin g_uiText+32 g_uiText+64
printf "UI WINDOW port=%08lx planes=%08lx parity=%u dma=%04x\n",g_uiWindow,g_displayPlanes,g_displayParity,*(unsigned short*)0xdff002
dump binary memory @RUN@/ui-window-planes.bin g_displayPlanes g_displayPlanes+196608
dump binary memory @RUN@/ui-window-copper.bin g_displayCopper g_displayCopper+2236
delete breakpoints
source @RUN@/core.gdb
