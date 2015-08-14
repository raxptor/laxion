package com.raxptor.laxion;

public class Main
{
	public static void main(String[] args) 
	{
		if (System.getProperty("os.name").contains("Windows")) {
			putked.Main.addPluginDesc(
					new putked.DefaultBuildLoader(Main.class, "interop.dll", "laxion-data-dll.dll", "laxion-databuilder.exe", new String[] {})
				);			
		} else {
			putked.Main.addPluginDesc(
					new putked.DefaultBuildLoader(Main.class, "interop.dylib", "liblaxion-data-dll.dylib", "laxion-databuilder", new String[] {})
				);
		}
		putked.Main.main(args);
	}
}
