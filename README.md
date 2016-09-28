# SecretKnock
Secret Knock Code - By Grathio


Code overview:

For the curious, here's a look at a few bits of code if you're interested in tinkering:
(If you're not curious, go to the next section)

about Line 28:const int threshold = 4; 

This is the sensitivity of the knock detector.  If you get a lot of noise, raise this (up to 1023), if you're having a hard time hearing knocks you can lower it (as low as 1).

about Line 29:const int rejectValue = 25; 
about Line 30:const int averageRejectValue = 15; 

Both of these are used to determine how accurately someone has to knock.  They are percentages and should be in the range of 0-100. Lowering these means someone must have more precise timing, higher is more forgiving.  averageRejectValue should always be lower than rejectValue. 

Settings of about 10 and 7 make it hard for two people to knock the same knock even if they know the rhythm. But it also increases the number of false negatives. (ie: You knock correctly and it still doesn't open.)

about Line 31:const int knockFadeTime = 150; 

This is a crude debounce timer for the knock sensor.  After it hears a knock it stops listening for this many milliseconds so it doesn't count the same knock more than once.  If you get a single knock counted as two then increase this timer.  If it doesn't register two rapid knocks then decrease it.

about Line 32:const int lockTurnTime = 650; 

This is now many milliseconds we run the motor to unlock the door.  How long this should be depends on the design of your motor and your lock.  It's okay if it runs a little bit long since I've designed a simple slip clutch into the design, but it's better for all the parts if it doesn't run too much.

about Line 34:const int maximumKnocks = 20;

How many knocks we record.  20 is a lot.  You can increase this if your secret hideout is protected by devious drummers with good memories.  Increase it too much and you'll run out of memory.

about Line 35:const int knockComplete = 1200;

Also known as the maximum number of milliseconds it will wait for a knock.  If it doesn't hear a knock for this long it will assume it's done and check to see if the knock is any good.  Increase this if you're a slow knocker.  Decrease it if you're a fast knocker and are impatient to wait 1.2 seconds for your door to unlock.

about Line 39:int secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 5.....

This is the default knock that it recognizes when you turn it on.  This is weird rhythmic notation since every value is a percentage of the longest knock.   If you're having a hard time getting it to recognize "shave and a hair cut" change this to {100,100,100,0,0,0...  and a simple sequence of 3 knocks will open it.

Debugging:
about Line 51:  Serial.begin(9600); 
about Line 52:Serial.println("Program start.");

Uncomment these lines to see some debug info on the serial port.  There are a few other lines of debugging code set throughout the rest of code that you can uncomment to see what's going on internally.

Be sure to set your serial port to the right speed.

The rest of the code is commented so you can see how it works but you probably won't need to change it if you aren't changing the design.
