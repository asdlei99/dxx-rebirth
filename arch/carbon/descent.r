
#include "Processes.r"

resource 'SIZE' (-1) {
	reserved,
	acceptSuspendResumeEvents,
	reserved,
	canBackground,
	doesActivateOnFGSwitch,
	backgroundAndForeground,
	getFrontClicks,
	ignoreAppDiedEvents,
	is32BitCompatible,
	isHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	useTextEditServices,
	reserved,
	reserved,
	reserved,
	30000*1024,		// 30 megs minimum
	50000*1024			// 50 megs maximum
};

data 'carb' (0) {};

data 'DLOG' (1000) {
	$"0072 0040 00EA 01B3 0001 0100 0000 0000 0000 03E8 0C43 6F6D 6D61 6E64 204C 696E"                    /* .r.@.�.�...........�.Command Lin */
	$"6500 280A"                                                                                          /* e.(� */
};

data 'DLOG' (1001) {
	$"0072 0040 00DB 01AC 0001 0100 0000 0000 0000 03E9 0C45 7272 6F72 2057 696E 646F"                    /* .r.@.�.�...........�.Error Windo */
	$"7700 280A"                                                                                          /* w.(� */
};

data 'DLOG' (1002) {
	$"00B8 00BE 0147 01D8 0005 0100 0000 0000 0000 03EA 1643 6F6E 6669 726D 2044 6973"                    /* .�.�.G.�...........�.Confirm Dis */
	$"706C 6179 2043 6861 6E67 6510 280A"                                                                 /* play Change.(� */
};

data 'DITL' (1000) {
	$"0005 0000 0000 0052 0113 0066 0158 0402 4F4B 0000 0000 0052 00C2 0066 0107 0406"                    /* .......R...f.X..OK.....R.�.f.... */
	$"4361 6E63 656C 0000 0000 000F 0084 001F 0155 1000 0000 0000 0054 0019 0066 007D"                    /* Cancel.......�...U.......T...f.} */
	$"050E 4F75 7470 7574 2074 6F20 6669 6C65 0000 0000 000F 0018 001F 007F 080D 436F"                    /* ..Output to file..............Co */
	$"6D6D 616E 6420 4C69 6E65 3A00 0000 0000 0030 0018 0040 0158 0702 0080"                              /* mmand Line:......0...@.X...� */
};

data 'DITL' (1001) {
	$"0001 0000 0000 0046 0120 005A 015A 0402 4F4B 0000 0000 0010 000A 0038 0160 0800"                    /* .......F. .Z.Z..OK.......�.8.`.. */
};

data 'DITL' (1002) {
	$"0002 0000 0000 006F 001E 0083 0058 0406 4361 6E63 656C 0000 0000 006E 00C0 0082"                    /* .......o...�.X..Cancel.....n.�.� */
	$"00FA 0402 4F4B 0000 0000 000E 000F 005F 010C 88B3 5468 6520 7365 7474 696E 6720"                    /* .�..OK........._..��The setting  */
	$"666F 7220 796F 7572 206D 6F6E 6974 6F72 2068 6173 2062 6565 6E20 6368 616E 6765"                    /* for your monitor has been change */
	$"642C 2061 6E64 2069 7420 6D61 7920 6E6F 7420 6265 2064 6973 706C 6179 6564 2063"                    /* d, and it may not be displayed c */
	$"6F72 7265 6374 6C79 2E20 546F 2063 6F6E 6669 726D 2074 6865 2064 6973 706C 6179"                    /* orrectly. To confirm the display */
	$"2069 7320 636F 7272 6563 742C 2063 6C69 636B 204F 4B2E 2054 6F20 7265 7475 726E"                    /*  is correct, click OK. To return */
	$"2074 6F20 7468 6520 6F72 6967 696E 616C 2073 6574 7469 6E67 2C20 636C 6963 6B20"                    /*  to the original setting, click  */
	$"4361 6E63 656C 2E00"                                                                                /* Cancel.. */
};

data 'MENU' (128, preload) {
	$"0080 0000 0000 0000 0000 FFFF FFFB 0114 0C41 626F 7574 2053 444C 2E2E 2E00 0000"                    /* .�........����...About SDL...... */
	$"0001 2D00 0000 0000"                                                                                /* ..-..... */
};

data 'MENU' (129) {
	$"0081 0000 0000 0000 0000 FFFF FFFF 0C56 6964 656F 2044 7269 7665 7219 4472 6177"                    /* .�........����.Video Driver.Draw */
	$"5370 726F 636B 6574 2028 4675 6C6C 7363 7265 656E 2900 0000 001E 546F 6F6C 426F"                    /* Sprocket (Fullscreen).....ToolBo */
	$"7820 2028 4675 6C6C 7363 7265 656E 2F57 696E 646F 7765 6429 0000 0000 00"                           /* x  (Fullscreen/Windowed)..... */
};

data 'CNTL' (128) {
	$"0000 0000 0010 0140 0000 0100 0064 0081 03F0 0000 0000 0D56 6964 656F 2044 7269"                    /* .......@.....d.�.�.....Video Dri */
	$"7665 723A"                                                                                          /* ver: */
};

data 'TMPL' (128, "CLne") {
	$"0C43 6F6D 6D61 6E64 204C 696E 6550 5354 520C 5669 6465 6F20 4472 6976 6572 5053"                    /* .Command LinePSTR.Video DriverPS */
	$"5452 0C53 6176 6520 546F 2046 696C 6542 4F4F 4C"                                                    /* TR.Save To FileBOOL */
};

