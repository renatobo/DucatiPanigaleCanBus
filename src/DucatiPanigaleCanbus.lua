    -- todo: add engine.wheelspeed

    local errorMsg = false
    local sensorname = "Ducati-Panigale"
    local sensordevicepattern = "DuCan-...."
    local bleservice = "6E400001-59f2-4a41-9acd-cd56fb435d64"
    -- local blecharacteristicstatic = "6E400010-59f2-4a41-9acd-cd56fb435d64"
    local blecharacteristicslow = "6E400011-59f2-4a41-9acd-cd56fb435d64"
    local blecharacteristicfast = "6E400012-59f2-4a41-9acd-cd56fb435d64"
	local blecharacteristicwrite = "6E400012-59f2-4a41-9acd-cd56fb435000"
    
	--	sensor.oninit() is mandatory to define for any sensor purpose
    --	it sets basic parameters and sets up communication
    
    function sensor.oninit()
		----tracecall(connect, "sensor.oninit()")
        
        --	set sensor parameters
		sensor.channelsets = { engine } 
		
		sensor.normupdaterate = 20 -- this is the expected update rate, used to generate warnings
		sensor.connectiontype = btle -- one of btle, bt, wifi, mfi
        
        --	set BTLE peripheral name pattern and service / characteristics we are interested in
		-- if Expert settings contain an override, use it - otherwise the default of this module is in sensordevicepattern
		local customsensordevicepattern = preferences.getstring("kCustomOBDBTLEPeripheral")
		-- regular expression, dot means any character. Taken from Expert settings
		
		if string.len(customsensordevicepattern) > 0 then
			sensor.btle.peripheralnamepattern = customsensordevicepattern
			sensor.nameprefix = customsensordevicepattern -- shown in Sensor List
		else
			sensor.btle.peripheralnamepattern = sensordevicepattern
			sensor.nameprefix = sensorname -- shown in Sensor List
		end

        --  set our read charactersitics, we will use the tag returned later
        -- not impletemented as it requires writing on bus - VIN
        -- no notification needed
        -- readcharacteristicstatic = sensor.btle.addcharacteristic(bleservice, blecharacteristicstatic,  false) 
        -- rpm, tps, gear
        readcharacteristicfast =   sensor.btle.addcharacteristic(bleservice, blecharacteristicfast,    true)
        -- enginetemperature, ambienttemperature, battery
        readcharacteristicslow =   sensor.btle.addcharacteristic(bleservice, blecharacteristicslow,    true)
		-- for write to configurations (used right now to restart the unit)
		writecharacteristicrestart = sensor.btle.addcharacteristic(bleservice, blecharacteristicwrite, false)
        
		-- fetch additional info from service such as battery or firmware
		sensor.btle.deviceinformation = true
        
        --	Engine channel specific settings
		sensor.engine.elm327 = false -- this is an important one: setting it to true will use predefined parsing
		sensor.engine.obdonly = false -- qualifies to support more than ODBII / needed?
		----tracereturn(connect, "sensor.oninit()")

		sensor.configurationdefinitions =
		{
			reboot = { label = "Restart", type = "boolean", default = false }
		}

	end

	function sensor.onconfigurationschanged()

		if sensor.configurations.reboot then
			sensor.btle.writevalue(writecharacteristicrestart, "1")
			trace(adhoc, "Reconfig set sensor.btle.onconfigurationschanged")
		else
			-- since there are no other cases, throw an error
			trace(adhoc, "FAILED Reconfig, we should not be here")
		end

	end

	--  sensor.onconnect () is optional and called by the framework once the sensor
	--  is connected to the app; this hook can be used to run custom initialization

	function sensor.onconnect ()
		----trace(connect, "Ducati Panigale connected...")

		errorMsg = false
		
		-- queue the channel sets in sensor.onconnect(), send 1 and max value to normalize % counters
		-- http://forum.gps-laptimer.de/viewtopic.php?f=47&t=4608

		enginechannelset = {}
		enginechannelset.tps=1
		sensor.queuechannelset(enginechannelset, engine)

		enginechannelset = {}
		enginechannelset.tps=100
		sensor.queuechannelset(enginechannelset, engine)
        
        -- read static characteristic, once
        -- sensor.btle.readvalue(readcharacteristicstatic)

		-- reset the reboot flag
		sensor.changeconfigurations({ reboot = false})
        
	end

	--  sensor.ondisconnect () is optional and called by the framework after the sensor
	--  has been disconnected from the app

	function sensor.ondisconnect ()
		----trace(connect, "Ducati Panigale disconnected...")
	end

	-- sensor.btle.onvaluechanged () is mandatory for btle sensor
	-- it needs to redirect incoming data either to custom processing,
	-- or dispatch it to one of the standard parsers; characteristic is a tag returned
	-- by sensor.btle.addcharacteristic (), value is a byte sequence

	function sensor.btle.onvaluechanged (characteristic, value)
		----tracecall(btle, "sensor.btle.onvaluechanged(" .. characteristic .. ", " .. ----tracebytes(value) .. ")")
		if characteristic == readcharacteristicfast then
			sensor.fastbytesread (value)
        elseif characteristic == readcharacteristicslow then
            sensor.slowbytesread (value)
        -- elseif characteristic == readcharacteristicstatic then
        --    sensor.staticbytesread (value)
        end
		----tracereturn(btle, "sensor.btle.onvaluechanged()")
	end

    function sensor.fastbytesread(message)
        -- rpm, tps, gear
		----tracecall(gnss, "sensor.bytesread(" .. ----tracebytes(message) .. ")")
		if #message > 1 then

			local messagetype
			messagetype = string.unpack("I1", message)

			if messagetype == 1 then

				-- Create a new and empty set of channels 
				enginechannelset = {}

				-- Parsing RPM, 2 bytes
				-- engine.rpm: engine rounds per minute (RPM); integer 0..16383; mandatory 
				-- wheelspeed 2 bytes
				-- engine.wheelspeed: wheel speed in km/h; integer; optional; 
				-- Parsing APS, 1 byte 
				-- engine.tps: throttle or pedal position; double 0..100 percent; mandatory; engine.throttle is a valid synonym
				-- Parsing gear, 1 byte 
				-- engine.gear [v23]: gear with -1 rear, and 0 neutral; integer; optional; usually derived from speed, rpms, gear
				-- and drive ratios, this channel can be used to feed in a gear measured 
				
				-- we use pos=2 to skip the message byte we just read
				enginechannelset.rpm, enginechannelset.wheelspeed, enginechannelset.tps, enginechannelset.gear = string.unpack("I2I2I1i1", message,2)
				-- Pass result to app
				sensor.queuechannelset(enginechannelset, engine)
				sensor.rawupdatedforsensortype(engine) -- Rate Update
			else
				if errorMsg == false then
					error("Unrecognized Fast message version: " .. messagetype .. ". Update your HLT script")
					errorMsg = true
				end
				sensor.queuechannelset(nil, engine)
			end
		else
			-- Signal app we have received an invalid fix
			if errorMsg == false then
				error("Unexpected Fast BLE message size: " .. #message .. " bytes. Update your HLT script")
				errorMsg = true
			end
			sensor.queuechannelset(nil, engine)
		end
		--tracereturn(engine, "sensor.fastbytesread()")
	end

    function sensor.slowbytesread(message)
        -- enginetemperature, ambientemperature, battery

		----tracecall(gnss, "sensor.slowbytesread(" .. ----tracebytes(message) .. ")")
		if #message > 1 then
			local messagetype
			messagetype = string.unpack("I1", message)

			if messagetype == 2 then
				
				-- Create a new and empty set of channels 
				enginechannelset = {}
				-- Parsing engine temp, byte 1
				-- engine.enginetemp: coolant temperature; integer -40..215 degree Celsius; optional
				-- Parsing environmental temp, byte 2
				-- engine.iat: intake air temperature; integer -40..215 degree Celsius; optional 
				-- engine.battery: voltage of board battery; double, 1.0 = 1V; optional; not stored permanently 
				
				local battery
				-- we use pos=2 to skip the message byte we just read
				enginechannelset.enginetemp, enginechannelset.iat, battery = string.unpack("I1I1I1", message,2)
				-- battery is read as unsigned int (0..256) as in the original source so we divide by ten
				enginechannelset.battery = battery / 10
				-- Pass result to app
				sensor.queuechannelset(enginechannelset, engine)
				-- sensor.rawupdatedforsensortype(engine) -- Rate Update, not updated to keep only the highest frequency of the other messages
			else 
				if errorMsg == false then
					error("Unrecognized Slow message version: " .. messagetype .. ". Update your HLT script")
					errorMsg = true
				end
				sensor.queuechannelset(nil, engine)
			end
		else
			-- Signal app we have received an invalid fix
			if errorMsg == false then
				error("Unexpected Slow BLE message size: " .. #message .. " bytes. Update your HLT script")
				errorMsg = true
			end
			sensor.queuechannelset(nil, engine)
		end
		--tracereturn(engine, "sensor.slowbytesread()")
    end

--    function sensor.staticbytesread(message)
--       -- VIN
--
--		----tracecall(gnss, "sensor.staticbytesread(" .. ----tracebytes(message) .. ")")
--		if #message > 16 then
--			-- Create a new and empty set of channels 
--           enginechannelset = {}    
--            -- no Parsing for VIN, we can simply read the message
--            enginechannelset.vin = message
--			-- Pass result to app
--			sensor.queuechannelset(enginechannelset, engine)
--		else
--			-- Signal app we have received an invalid VIN
--			if errorMsg == false then
--				error("Invalid VIN BLE message size: " .. #message .. " bytes")
--				errorMsg = true
--			end
--			sensor.queuechannelset(nil, engine)
--		end
--		--tracereturn(engine, "sensor.staticbytesread()")
--	end