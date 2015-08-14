package com.raxptor.laxion;

import java.io.File;

class DevLoader implements putked.EditorPluginDescription
{
	@Override
	public String getName() { return "DEV Editor"; }
	@Override
	public String getVersion() { return "None"; }
	@Override
	public PluginType getType() { return PluginType.PLUGIN_PROJECT_DEV_BUILD; }
	@Override
	public void start() {
		File projRoot = new File(putked.Main.getProjectPath());
		File interop = new File(projRoot, "ext/putki/putked/interopdll/libputked-java-interop.dylib");
		File dll = new File(projRoot, "build/liblaxion-data-dll.dylib");
		File builder = new File(projRoot, "build/laxion-databuilder");
		putked.Main.interopInit(interop.getAbsolutePath(), dll.getAbsolutePath(), builder.getAbsolutePath());
	}
}

public class DevMain
{
	public static void main(String[] args) 
	{
		putked.Main.addPluginDesc(new DevLoader());
		putked.Main.main(args);
	}
}
