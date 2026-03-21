<?xml version='1.0' encoding='UTF-8'?>
<Project Type="Project" LVVersion="25008000">
	<Property Name="NI.LV.All.SaveVersion" Type="Str">25.0</Property>
	<Property Name="NI.LV.All.SourceOnly" Type="Bool">true</Property>
	<Item Name="My Computer" Type="My Computer">
		<Property Name="server.app.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.control.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.tcp.enabled" Type="Bool">false</Property>
		<Property Name="server.tcp.port" Type="Int">0</Property>
		<Property Name="server.tcp.serviceName" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.tcp.serviceName.default" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.vi.callsEnabled" Type="Bool">true</Property>
		<Property Name="server.vi.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="specify.custom.address" Type="Bool">false</Property>
		<Item Name="#typedef States.ctl" Type="VI" URL="../Controls/#typedef States.ctl"/>
		<Item Name="Bitcrush.vi" Type="VI" URL="../SubVi/Bitcrush.vi"/>
		<Item Name="Data.ctl" Type="VI" URL="../Data.ctl"/>
		<Item Name="Envelope.vi" Type="VI" URL="../SubVi/Envelope.vi"/>
		<Item Name="GenerateWave.vi" Type="VI" URL="../SubVi/GenerateWave.vi"/>
		<Item Name="main.vi" Type="VI" URL="../main.vi"/>
		<Item Name="status.ctl" Type="VI" URL="../Controls/status.ctl"/>
		<Item Name="Dependencies" Type="Dependencies"/>
		<Item Name="Build Specifications" Type="Build"/>
	</Item>
</Project>
