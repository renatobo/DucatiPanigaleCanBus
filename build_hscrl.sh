#!/bin/bash

output="scripts/DucatiPanigaleCanbus.hscrl"
input="src/DucatiPanigaleCanbus.lua"
cat << EOF > $output
<?xml version="1.0" encoding="utf-8"?>
<!--

	description

 -->
<scriptdefinition
	xmlns="http://www.gps-laptimer.de"
	xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	xsi:schemaLocation="http://www.gps-laptimer.de Scripts.xsd">

	<author>Renato Bonomini</author>
	<version>0.3</version>
	<purpose>sensor.btle</purpose>
	<replacing>false</replacing>
	<script><![CDATA[
EOF

cat $input >> $output

cat << EOF >> $output

	]]></script>
</scriptdefinition>
EOF