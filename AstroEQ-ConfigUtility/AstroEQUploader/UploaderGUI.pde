
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.nio.channels.FileChannel;
import javax.swing.*;
import java.util.LinkedHashMap;
import java.awt.Component;

class UploaderGUI {
  protected PApplet parent;
  private ControlP5 _control;
  private Dimensions dimensionMaker;
  private final static int DRIVER_A498x = 0;
  private final static int DRIVER_DRV882x = 1;
  private final static int DRIVER_DRV8834 = 2;
  private final static int DRIVER_COUNT = 3;
  private final static int MIN_IVAL = 300; //normal minimum
  private final static int ABSOLUTE_MIN_IVAL = 50; //absolute minimum below which an error occurs.
  private int[] minimumIVal = {MIN_IVAL,MIN_IVAL}; //actual minimum
  int screenStatus = 0;
  public Boolean testRun = false;
  private String[] axisIDStrings; //initialised when making config screen.
  private String[] extraArgs = null;
  private final static int GUI_BEGIN = 0;
  private final static int GUI_CHECK_VERSION = GUI_BEGIN+1;
  private final static int GUI_FETCH_FIRMWARE = GUI_CHECK_VERSION+1;
  private final static int GUI_BURN_UTILITY = GUI_FETCH_FIRMWARE+1;
  private final static int GUI_EEPROM_READ = GUI_BURN_UTILITY+1;
  private final static int GUI_EEPROM_RECOVER = GUI_EEPROM_READ+1;
  private final static int GUI_ENTER_CONFIG = GUI_EEPROM_RECOVER+1;
  private final static int GUI_STORE_CONFIG = GUI_ENTER_CONFIG+1;
  private final static int GUI_BURN_ASTROEQ = GUI_STORE_CONFIG+1;
  private final static int GUI_LOAD_EQMOD = GUI_BURN_ASTROEQ+1;
  private final static int GUI_FINISHED = GUI_LOAD_EQMOD+1;
  private final static int GUI_TOTAL_PAGES = GUI_FINISHED+1;
  private String[] groupNames;
  private String[] groupTitles;
  
  Serial configSerialPort = null;
  ConcurrentLinkedQueue<Character> inQueue = null;
  ConcurrentLinkedQueue<Character> outQueue = null;
 
  private String calculateGotoFactor(String IValStr, String MultiplierStr, boolean microstepping, int minIVal){
    int IVal;
    int Multiplier;
    try {
      IVal = Integer.parseInt(IValStr);
      Multiplier = Integer.parseInt(MultiplierStr);
    } catch (Exception e) {
      return null;
    }
    if (Multiplier == 0 || Multiplier > 255 || IVal > 1200 || IVal < minIVal) {
      return null;
    }
    int Factor = (int)Math.round((double)IVal / (double)Multiplier);
    if (microstepping){
      Factor *= 8; //Goto speed is 8x larger for microstepping
    }
    Factor = Math.min(Factor,1200); //limit max speed
    return String.valueOf(Factor);
  }
  private String getMinimumGotoFactor(String IValStr, boolean microstepping, int minIVal){
    int IVal;
    try {
      IVal = Integer.parseInt(IValStr);
    } catch (Exception e) {
      return null;
    }
    if (IVal > 1200 || IVal < minIVal) {
      return null;
    }
    int Factor = (int)Math.ceil((double)IVal/255.0);
    if (microstepping){
      Factor *= 8; //Goto speed is 8x larger for microstepping
    }
    Factor = Math.min(Factor,1200); //limit max speed
    return String.valueOf(Factor);
  }
  private String calculateGotoMultiplier(String IValStr, String FactorStr, boolean microstepping, int minIVal){
    int IVal;
    double Factor;
    try {
      IVal = Integer.parseInt(IValStr);
      Factor = Float.parseFloat(FactorStr);
    } catch (Exception e) {
      return null;
    }
    if (microstepping){
      Factor /= 8; //Goto speed is 8x larger for microstepping
    }
    if ((Factor < 1) || (IVal > 1200) || (IVal < minIVal)) {
      return null;
    }
    
    int Multiplier = (int)Math.round((double)IVal / Factor);
    if ((Multiplier > 255)) {
      return null; //multiplier not in range
    }
    if (Multiplier == 0) {
      Multiplier = 1; //Must not be zero.
    }    
    return String.valueOf(Multiplier);
  }
 
  private boolean generateAccelerationTable(short IVal, int bVal, int ACCEL_TABLE_LEN, int MAX_REPEATS, short[] IVals, byte[] repeats, String axisNameDebug) {
    short startIVal = (short)(IVal/2);
    double stepPerSec = (double)bVal;
    double secPerStep = 1.0 / (double)bVal;
    
    short stopIVal = 2;
    double accelTime = 4.0;
    
    println("Generating Acceleration Table for "+axisNameDebug);
    
    int attemptNo = 1;
    
    while (stopIVal <= 8) {
      
      double startSpeed = (stepPerSec / (double)startIVal);
      double endSpeed = (stepPerSec / (double)stopIVal);
      
      //Calculate acceleration parameters for linear profile -> Y = mX + c
      double m = (endSpeed - startSpeed) / accelTime;
      double c = startSpeed;
      
      //Start us off at the start IVal
      int currentIndex = 0;
      double currentTime = 0;
      IVals[currentIndex] = startIVal; //First IVal is the startIVal
      repeats[currentIndex] = 1;       //And we will have 1 step of that.
      double currentSpeed = startSpeed; //Current speed is the starting speed.
      
      while (currentIndex < ACCEL_TABLE_LEN-1) {
        //Move time on as we have performed another step.
        currentTime = currentTime + (secPerStep * (double)IVals[currentIndex]);
        //Calculate the required velocity at this moment in time
        double requiredVelocity = m * currentTime + c;
        if ((currentSpeed >= requiredVelocity) && (repeats[currentIndex] < MAX_REPEATS)) {
          //If we are going faster than required, and we have not used up all of our repeats, do another step at this speed.
          repeats[currentIndex]++;
        } else {
          //Otherwise we need to determine what speed to go at next.
          short nextIVal = IVals[currentIndex];
          while (currentSpeed < requiredVelocity) {
            //While the current speed is too slow, calculate the next available speed
            nextIVal--; //Next speed is at the next IVal.
            currentSpeed = (stepPerSec / (double)nextIVal); //Convert to a speed
          }
          currentIndex++; //New speed means a new index
          IVals[currentIndex] = nextIVal; //store the new IVal
          repeats[currentIndex] = 1; //And start off with 1 step of it.
          
        }
        if (IVals[currentIndex] == stopIVal) {
          //If we have hit the stopping speed, then the table is complete.
          //Pad the table with copies of the last element until it is the full length
          for (int i = currentIndex + 1; i < ACCEL_TABLE_LEN; i++) {
            IVals[i] = IVals[i-1];
            repeats[i] = repeats[i-1];
          }
          //Increase the first few repeat counts to reduce jerk.
          repeats[0] = (byte)(repeats[0] + 2);
          repeats[1] = (byte)(repeats[1] + 1);
          //And done
          println("Table Generation Success:");
          for (int i = 0; i < ACCEL_TABLE_LEN; i++) {
            println("Entry " + i + ": IVal = " + IVals[i] + ", steps = " + repeats[i]);
          }
          
          return true; //Table found and populated.
        }
      }
      
      //If we reach here, we failed to find a table.
      println("Attempt " + attemptNo + " Failed. Abandoned at IVal: " + IVals[currentIndex]);
      if (accelTime > 0.85) {
        //Otherwise try accelerating faster
        accelTime -= 0.1;
      } else {
        //So lets try again with a slightly slower top speed if we haven't slowed it down too far already
        stopIVal++;
        //And reset accel time for next time.
        accelTime = 4.0;
      }
      println("Trying Top Speed IVal of " + stopIVal + " and Accel Time of " + accelTime);
    }
    return false; //Failed to find a table.
  }
  
  
  
  private void arrayInitialise(){
    groupNames = new String[GUI_TOTAL_PAGES];
    groupTitles = new String[GUI_TOTAL_PAGES];
    groupNames[GUI_BEGIN] = "begin";
    groupNames[GUI_CHECK_VERSION] = "versionCheck";
    groupNames[GUI_FETCH_FIRMWARE] = "firmwareFetch";
    groupNames[GUI_BURN_UTILITY] = "EEPROMWriter";
    groupNames[GUI_EEPROM_READ] = "checkEEPROM";
    groupNames[GUI_EEPROM_RECOVER] = "recoverEEPROM";
    groupNames[GUI_ENTER_CONFIG] = "enterConfig";
    groupNames[GUI_STORE_CONFIG] = "programConfig";
    groupNames[GUI_BURN_ASTROEQ] = "downloadFirmware";
    groupNames[GUI_LOAD_EQMOD] = "testFirmware";
    groupNames[GUI_FINISHED] = "finished";
    groupTitles[GUI_BEGIN] = "Select a task...";
    groupTitles[GUI_CHECK_VERSION] = "Check for New Firmware Version";
    groupTitles[GUI_FETCH_FIRMWARE] = "Fetching Latest AstroEQ Firmware";
    groupTitles[GUI_BURN_UTILITY] = "Uploading EEPROM Reader Firmware...";
    groupTitles[GUI_EEPROM_READ] = "Reading Configuration From EEPROM...";
    groupTitles[GUI_EEPROM_RECOVER] = "Rebuilding EEPROM Config File";
    groupTitles[GUI_ENTER_CONFIG] = "AstroEQ Configuration Setup";
    groupTitles[GUI_STORE_CONFIG] = "Storing Configuration to EEPROM";
    groupTitles[GUI_BURN_ASTROEQ] = "Programming AstroEQ Firmware...";
    groupTitles[GUI_LOAD_EQMOD] = "Testing AstroEQ Communication";
    groupTitles[GUI_FINISHED] = "Firmware Update Finished";
  }

  //private GUIScreen[] groups;
  protected Map<String, GUIScreen> groups = new HashMap<String, GUIScreen>();
  
  private int currentGroupIndex = (__TESTING__==2) ? GUI_ENTER_CONFIG : GUI_BEGIN;
  
  UploaderGUI(PApplet parent, ControlP5 control, Dimensions dimension) {
    this.parent = parent;
    _control = control;
    arrayInitialise();
    println("x="+dimension.x()+", y="+dimension.y()+", width="+dimension.width()+", height="+dimension.height());
    dimensionMaker = new Dimensions(dimension.x(), dimension.y());
    dimensionMaker.setSize(dimension.width(), dimension.height());

    for (int i = 0; i < groupNames.length; i++) {
      GUIScreen whichScreen = new GUIScreen(control, groupNames[i], groupTitles[i], dimensionMaker);
      groups.put(groupNames[i], whichScreen);

      Method method;
      try {
        String methodName = "build" + Character.toUpperCase(groupNames[i].charAt(0)) + groupNames[i].substring(1) + "Screen";
        print(methodName + " --- ");
        method = this.getClass().getDeclaredMethod(methodName, control.getClass(), whichScreen.getClass());
        method.invoke(this, control, whichScreen);
      } catch (Exception e) {
        println("Exception!!");//There could be lots of different exceptions, but they all result in me skipping the call.
        continue;
      }
      setHidden(i);
    }

    setVisible(currentGroupIndex);
    
  }

  public ControlP5 getControl() {
    return _control;
  }
  
  private int getMicrostepMode(){
    GUIScreen curScreen = groups.get(groupNames[currentGroupIndex]);
    Integer usteps;
    if ((usteps = (Integer)curScreen.getScrollableListItem("ramicrostepEnableField")) != null) {
      return (int)usteps;
    }
    return 1;
  }
  
  private boolean canJumpToHighspeed(){
    GUIScreen curScreen = groups.get(groupNames[currentGroupIndex]);
    Button gearChangeButton = curScreen.getButton("ragearchangeEnableField");
    if (!gearChangeButton.isOn()){
      return false; //No high speed if gear change disabled
    }
    return (getMicrostepMode()>=8); //Otherwise high speed if the microstep mode is >= 8.
  }
  
  private boolean openSerialPort() {
    try {
      configSerialPort = new Serial(parent, curPort, 9600);
      inQueue = new ConcurrentLinkedQueue<Character>();
      outQueue = new ConcurrentLinkedQueue<Character>();
    } catch (Exception e) {
      configSerialPort = null;
      inQueue = null;
      outQueue = null;
      return false;
    }
    configSerialPort.clear();
    return true;
  }
  
  private void closeSerialPort() {
    if (configSerialPort != null) {
      configSerialPort.clear();
      configSerialPort.stop();
      configSerialPort = null;
    }
    inQueue = null;
    outQueue = null;
  }
  
  private void processSerialData() {
    if (configSerialPort != null) {
      if (inQueue != null) {
        while(configSerialPort.available() > 0) {
          inQueue.offer(new Character((char)configSerialPort.read()));
        }
      } else {
        configSerialPort.clear();
      }
      if (outQueue != null) {
        Character ch;
        while((ch = outQueue.poll()) != null) {
          configSerialPort.write(ch.charValue());
        }
      }
    }
  }
    
  public boolean dropdownEvent(ControlEvent theEvent){ 
    String name = "";
    boolean found = false;
    for (String id : groupNames) {
      name = theEvent.getName();
      if (name.length() < id.length()) {
        continue; //can't possibly be this id.
      }
      name = name.substring(0, id.length());
      if (id.equals(name)) {
        found = true;
        break;
      }
    }
    if (found && name.equals(groupNames[currentGroupIndex])) {
      //a matching ID was found, and it is the current view, so name now contains the id of a view.
      name = theEvent.getName().substring(name.length());
      //println(name);
  
      GUIScreen currentScreen = groups.get(groupNames[currentGroupIndex]);
      //Handle the event.
      //println(theEvent.getValue());
      
      switch (currentGroupIndex) {
        case GUI_ENTER_CONFIG:
          if (name.equals("ramicrostepEnableField")){
            Button gearChangeButton = currentScreen.getButton("ragearchangeEnableField");
            int usteps = getMicrostepMode();
            int fusteps = usteps;
            String infoToPrint = "";
            if ((usteps < 8) && gearChangeButton.isOn()) {
              infoToPrint = "The ustep mode is <8, \"uStep Gear Changing\" is Disabled\n";
              gearChangeButton.setCaptionLabel("Disabled");
              gearChangeButton.setOff(); //Setting this will trigger button to appear pressed which will print ustep mode.
              gearChangeButton.lock(); //Prevent changing.
            } else {
              infoToPrint = "Step Modes: Slow = "+usteps+(usteps>1?" usteps":" ustep")+"/stp";
              if (usteps >=8) {
                gearChangeButton.unlock(); //Allow changing.
                if(gearChangeButton.isOn()) {
                  fusteps /= 8;
                }
              } else {
                gearChangeButton.lock(); //Prevent changing.
              }
              infoToPrint = infoToPrint + ", Fast = "+usteps+(usteps>1?" usteps":" ustep")+"/step\n";
            }
            //print out the current microstep mode.
            info.setText(info.getText()+infoToPrint);
            info.scroll(1);
          } else if (name.substring(2).equals("driverVersionField")) {
            ScrollableList versionDropdown = currentScreen.getDropdown(name);
            ScrollableList microstepDropdown = currentScreen.getDropdown("ramicrostepEnableField");
            if ((versionDropdown != null) && (microstepDropdown != null)){
              if ((Integer)currentScreen.getScrollableListItem(versionDropdown) != DRIVER_A498x){
                microstepDropdown.removeItem("32 uStep");//ensure there aren't duplicates
                microstepDropdown.addItem("32 uStep", 32);
              } else {
                if ((Integer)currentScreen.getScrollableListItem(microstepDropdown)==32){
                  currentScreen.selectScrollableListItem(microstepDropdown,16);
                }
                microstepDropdown.removeItem("32 uStep");
              }
            }
          }
        default:
          break;
      }
      
    }
    return found;
  }
  
  private String[] loadDefaultConfiguration() {
    String[] newArgs = new String[]{ 
      ":a1",
      ":b1",
      ":s1",
      ":n1",
      ":a2",
      ":b2",
      ":s2",
      ":n2",
      ":c1",
      ":c2",
      ":r10.25",
      ":d11",
      ":d24",
      ":q10",
      ":q21",
      ":z1",
      ":z2",
      ":V1",
      ":V2",
      ":U1",
      ":U2",
      ":W1",
      ":W2"
    };
    List<String> args = new ArrayList<String>();
    
    for (String arg : newArgs) {
      boolean isRaDec = true;
      Object[] responseDecoder = responseLookup.get(""+arg.charAt(1));
      if (responseDecoder == null){
        responseDecoder = responseLookup.get(arg.substring(1,3));
        isRaDec = false;
      }
      if (responseDecoder != null){
        //boolean decodeLength = (Boolean)responseDecoder[1];
        String decodedValue = arg.substring(3);
        String decodedString = ((!isRaDec || (arg.charAt(2)=='1'))?"ra":"dc")+(String)responseDecoder[0]+decodedValue;
        args.add(decodedString);
      }
    }
    return args.toArray(new String[args.size()]);
  }
  
  public boolean controlEvent(ControlEvent theEvent) {
    String name = "";
    boolean found = false;
    for (String id : groupNames) {
      name = theEvent.getName();
      if (name.length() < id.length()) {
        continue; //can't possibly be this id.
      }
      name = name.substring(0, id.length());
      if (id.equals(name)) {
        found = true;
        break;
      }
    }
    if (found && name.equals(groupNames[currentGroupIndex])) {
      //a matching ID was found, and it is the current view, so name now contains the id of a view.
      name = theEvent.getName().substring(name.length());
      //println(name);

      GUIScreen currentScreen = groups.get(groupNames[currentGroupIndex]);
      //Handle the event.
      //println(theEvent.getValue());
      if (theEvent.getValue() != 1.0) {
        return found; //not interested in release, only press.
      }
      switch (currentGroupIndex) {
      case GUI_BEGIN:
        testRun = false;
        if (name.equals("configButton")) {
          setDisplay(GUI_EEPROM_READ);
        } else if (name.equals("uploadButton")) {
          setDisplay(GUI_BURN_ASTROEQ);
        } else if (name.equals("firmwareButton")) {
          setDisplay(GUI_CHECK_VERSION);
        } else if (name.equals("testButton")) {
          setDisplay(GUI_ENTER_CONFIG);
          testRun = true;
          extraArgs = loadDefaultConfiguration();
        } else if (name.equals("eqmodButton")) {
          if (isWindows()) {
            setDisplay(GUI_LOAD_EQMOD);
          } else if (isUnix()) {
            setDisplay(GUI_FINISHED);
          }
        }
        screenStatus = 0;
        info.setText("");
        execute.resetStatus(); //finished with the thread, so clear.
        break;
        
      case GUI_CHECK_VERSION:
        if (name.equals("nextButton")) {
          if (screenStatus == 6) {
            exit();
          } else if (screenStatus == 1) {
            break; //haven't yet checked
          } else if (screenStatus == 2) {
            setDisplay(GUI_FETCH_FIRMWARE); //new firmware version found
            String output = info.getText();
            println(output);
            if ((info.getText()!= null) && (output.length() > 0)) {
              extraArgs = output.split("\n"); //backup the list of files to fetch.
            }
          } else if (screenStatus < 5) {
            setDisplay(GUI_BEGIN); //no new firmware version found or no internet access
          } else if (screenStatus != 5) {
            break; //haven't yet checked
          }
          
          execute.resetStatus(); //finished with the thread, so clear.
          if (screenStatus == 5) {
            screenStatus = 1;
          } else {
            screenStatus = 0;
          }
          info.setText("");
        }
        break;
        
      case GUI_FETCH_FIRMWARE:
        if (name.equals("nextButton")) {
          if (screenStatus < 2) {
            break; //haven't yet checked
          } else if (screenStatus != 5) {
            versionListDrop(version, listVersions()); //update version displayed by firmware dropdown
            setDisplay(GUI_BEGIN); //got new firmware version or skipping
          }
          
          execute.resetStatus(); //finished with the thread, so clear.
          if (screenStatus == 5) {
            screenStatus = 1;
          } else {
            screenStatus = 0;
          }
          info.setText("");
        }
        break;
        
      case GUI_BURN_UTILITY:
        if (name.equals("nextButton")) {
        
          if (screenStatus == 1) {
            setDisplay(GUI_BEGIN);
          } else if (screenStatus == 2) {
            setDisplay(GUI_EEPROM_READ);
          } else {
            break;
          }

          execute.resetStatus(); //finished with the thread, so clear.
          screenStatus = 0;
          info.setText("");
        }
        break;

      case GUI_EEPROM_READ:
        if (name.equals("nextButton")) {
          
          if (screenStatus == 3) {
            setDisplay(GUI_BEGIN); //return to menu.
          } else if (screenStatus == 2) {
            setDisplay(GUI_EEPROM_RECOVER); //recover the EEPROM
          } else if (screenStatus == 4) {
            setDisplay(GUI_ENTER_CONFIG); //skip EEPROM recovery
            String output = info.getText();
            println(output);
            if ((info.getText()!= null) && (output.length() > 0)) {
              extraArgs = output.split("\n");
            }
            println("------ Configuration Data Extraction ------");
          } else if (screenStatus != 5) {
            break;
          }
          
          execute.resetStatus(); //finished with the thread, so clear.
          
          if (screenStatus == 5) {
            screenStatus = 1;
          } else {
            screenStatus = 0;
          }
          info.setText("");
        }
        break;

      case GUI_EEPROM_RECOVER:
        if (name.equals("nextButton")) {

          if (screenStatus == 1) {
            setDisplay(GUI_BEGIN);
          } else if (screenStatus == 2) {
            setDisplay(GUI_ENTER_CONFIG);
          } else {
            break;
          }

          execute.resetStatus(); //finished with the thread, so clear.
          
          if (screenStatus != 1) {
            extraArgs = loadDefaultConfiguration();
          }
          screenStatus = 0;
          info.setText("");
        }
        break;

      case GUI_ENTER_CONFIG:
        if (name.equals("resetButton")){
          println("------ Configuration Reset ------");
          extraArgs = loadDefaultConfiguration();
        } else if (name.equals("loadButton")){
          println("------ Configuration Load ------");
          SwingUtilities.invokeLater(new Runnable() {
            public void run() {
              try {
                // create a file chooser 
                final JFileChooser fc = new JFileChooser(filePath + java.io.File.separator + "mounts"); 
                FileFilter ffilter = new ExtensionFilter("AstroEQ Config File", new String[] {".conf"});
                fc.removeChoosableFileFilter(fc.getFileFilter());
                fc.addChoosableFileFilter(ffilter);
                fc.setFileFilter(ffilter);
      
                // in response to a button click: 
                int returnVal = fc.showOpenDialog(null); 
                 
                if (returnVal == JFileChooser.APPROVE_OPTION) { 
                  File file = fc.getSelectedFile(); 
                  
                  Boolean foundGearChangeField = false;
                  Boolean foundMicrostepField = false;
                  Boolean foundAdvancedHCEnableField = false;
                  
                  info.setText(info.getText()+"Loading: " + file.getName()+(file.getName().endsWith(".conf") ? "\n" : ".conf\n"));
                  println("Loading: "+file.getName()); 
                  
                  String loadName = file.getPath();
                  if (!loadName.endsWith(".conf")){
                    loadName = loadName + ".conf"; //append extension if required.
                  }
                  List<String> args = new ArrayList<String>();
                  BufferedReader br = new BufferedReader(new FileReader(loadName));
                  String arg;
                  while ((arg = br.readLine()) != null) {
                    boolean isRaDec = true;
                    Object[] responseDecoder = responseLookup.get(""+arg.charAt(1));
                    if (responseDecoder == null){
                      responseDecoder = responseLookup.get(arg.substring(1,3));
                      isRaDec = false;
                    }
                    if (responseDecoder != null){
                      //boolean decodeLength = (Boolean)responseDecoder[1];
                      String decodedValue = arg.substring(3);
                      String decodedString = ((!isRaDec || (arg.charAt(2)=='1'))?"ra":"dc")+(String)responseDecoder[0]+decodedValue;
                      args.add(decodedString.replace(',', '.'));
                      //println(decodedString);
                      if (responseDecoder[0].equals("gearchangeEnable")) {
                        foundGearChangeField = true;
                      } else if (responseDecoder[0].equals("advancedHCEnable")) {
                        foundAdvancedHCEnableField = true;
                      } else if (responseDecoder[0].equals("microstepEnable")) {
                        foundMicrostepField = true;
                      }
                    }
                  }
                  br.close();
                  if (!foundGearChangeField) {
                    //We may not have a gear change field if the config file is old. 
                    //If we don't assume it is enabled (old equivalent). If this is a non-allowed value it will be corrected later.
                    args.add("ragearchangeEnable0");
                    println("uStep Gear Changing setting not found. Using default.");
                  }
                  if (!foundAdvancedHCEnableField) {
                    //If no advanced HC enable value found, assume disabled.
                    args.add("raadvancedHCEnable0");
                  }
                  if (!foundMicrostepField) {
                    //We must have a microstep enable field. If there isn't one, default to 4.
                    args.add("ramicrostepEnable4");
                    println("uStep mode setting not found. Using default.");
                  }
                  extraArgs = args.toArray(new String[args.size()]);
                  println("Load successful!"); 
                  info.setText(info.getText()+"Load successful!" + "\n");
                } else { 
                  info.setText(info.getText()+"Loading of file cancelled by User." + "\n");
                  println("Open command cancelled by user."); 
                }
              } catch (Exception e) { 
                //e.printStackTrace();  
                info.setText(info.getText()+"File Load Failed with Exception" + "\n");
                println("File Load Failed with Exception"); 
              } 
            }
          });
        } else if (name.equals("saveButton")){
          println("------ Configuration Save ------");
          SwingUtilities.invokeLater(new Runnable() {
            public void run() {
              try {
                // create a file chooser 
                final JFileChooser fc = new JFileChooser(filePath + java.io.File.separator + "mounts"); 
                FileFilter ffilter = new ExtensionFilter("AstroEQ Config File", new String[] {".conf"});
                fc.removeChoosableFileFilter(fc.getFileFilter());
                fc.addChoosableFileFilter(ffilter);
                fc.setFileFilter(ffilter);
      
                // in response to a button click: 
                int returnVal = fc.showSaveDialog(null); 
                 
                if (returnVal == JFileChooser.APPROVE_OPTION) { 
                  File file = fc.getSelectedFile(); 
                  
                  GUIScreen curScreen = groups.get(groupNames[currentGroupIndex]);
                  String raGotoStr = calculateGotoMultiplier(curScreen.getField("raIValField").getText(),curScreen.getField("raGotoField").getText(),canJumpToHighspeed(),minimumIVal[Arrays.asList(axisIDStrings).indexOf("ra")]);
                  String dcGotoStr = calculateGotoMultiplier(curScreen.getField("dcIValField").getText(),curScreen.getField("dcGotoField").getText(),canJumpToHighspeed(),minimumIVal[Arrays.asList(axisIDStrings).indexOf("dc")]);
                                    
                  info.setText(info.getText()+"Saving: " + file.getName()+(file.getName().endsWith(".conf") ? "\n" : ".conf\n"));
                  println("Saving: " + file.getName()+(file.getName().endsWith(".conf") ? "" : ".conf"));
                  String[] toSave = new String[]{ 
                    ":a1"+ curScreen.getField("raaValField").getText(),
                    ":b1"+ curScreen.getField("rabValField").getText(),
                    ":s1"+ curScreen.getField("rasValField").getText(),
                    ":n1"+ curScreen.getField("raIValField").getText(),
                    ":a2"+ curScreen.getField("dcaValField").getText(),
                    ":b2"+ curScreen.getField("dcbValField").getText(),
                    ":s2"+ curScreen.getField("dcsValField").getText(),
                    ":n2"+ curScreen.getField("dcIValField").getText(),
                    ":c1"+ (curScreen.getButton("raaxisReverseField").isOn() ? "1" : "0"), 
                    ":c2"+ (curScreen.getButton("dcaxisReverseField").isOn() ? "1" : "0"), 
                    ":d1"+ (Integer)curScreen.getScrollableListItem("radriverVersionField"),
                    ":d2"+ (Integer)curScreen.getScrollableListItem("ramicrostepEnableField"), 
                    ":q1"+ (curScreen.getButton("raadvancedHCEnableField").isOn() ? "1" : "0"),
                    ":q2"+ (curScreen.getButton("ragearchangeEnableField").isOn() ? "0" : "1"),
                    ":r1"+ curScreen.getField("raGuideRateField").getText(),
                    ":z1"+ raGotoStr,
                    ":z2"+ dcGotoStr,
                    ":V1"+ curScreen.getField("raAngleField").getText(),
                    ":V2"+ curScreen.getField("dcAngleField").getText(),
                    ":U1"+ curScreen.getField("raUGearField").getText(),
                    ":U2"+ curScreen.getField("dcUGearField").getText(),
                    ":W1"+ curScreen.getField("raWGearField").getText(),
                    ":W2"+ curScreen.getField("dcWGearField").getText()
                  };
                  String saveName = file.getPath();
                  if (!saveName.endsWith(".conf")){
                    saveName = saveName + ".conf"; //append extension if required.
                  }
                  FileWriter writer = new FileWriter(saveName);
                  for (String arg : toSave){
                    writer.write(arg+"\n");
                  }
                  writer.close();
                  println("Save successful!"); 
                  info.setText(info.getText()+"Save successful!" + "\n");
                } else { 
                  info.setText(info.getText()+"Saving of file cancelled by User." + "\n");
                  println("Save command cancelled by user."); 
                }
              } catch (Exception e) { 
                //e.printStackTrace();  
                info.setText(info.getText()+"File Save Failed with Exception" + "\n");
                println("File Save Failed with Exception"); 
              } 
            }
          });
        } else if (name.substring(2).equals("UpdateButton")) {
          String id = name.substring(0, 2);
          println("------ "+id.toUpperCase()+" Axis Update ------");
          String angleString = currentScreen.getField(id+"AngleField").getText();
          String gearRatioString = currentScreen.getField(id+"UGearField").getText();
          String wormString = currentScreen.getField(id+"WGearField").getText();
          if ((angleString == null) || (angleString.length()==0)) {
            angleString = "0";
            currentScreen.getField(id+"AngleField").setText(angleString);
          }
          if ((gearRatioString == null) || (gearRatioString.length()==0)) {
            gearRatioString = "0";
            currentScreen.getField(id+"UGearField").setText(gearRatioString);
          }
          if ((wormString == null) || (wormString.length()==0)) {
            wormString = "0";
            currentScreen.getField(id+"WGearField").setText(wormString);
          }
          println("Step Angle: "+angleString);
          println("Motor Gear Ratio: "+gearRatioString);
          println("Worm Gear Ratio: "+wormString);
          double angle;
          try {
            angle = Double.parseDouble(angleString);
          } catch (NumberFormatException e) {
            angle = 0;
            currentScreen.getField(id+"AngleField").setText("0");
          }
          double gearRatio;
          try {
            gearRatio = Double.parseDouble(gearRatioString);
          } catch (NumberFormatException e) {
            gearRatio = 0;
            currentScreen.getField(id+"UGearField").setText("0");
          }
          double wormRatio;
          try {
            wormRatio = Double.parseDouble(wormString);
          } catch (NumberFormatException e) {
            wormRatio = 0;
            currentScreen.getField(id+"WGearField").setText("0");
          }
          String aValStr;
          String bValStr;
          String sValStr;
          String IValStr;
          String ResolutionStr;
          if ((wormRatio == 0) || (gearRatio == 0) || (angle == 0)) {
            aValStr = "0";
            bValStr = "0";
            sValStr = "0";
            IValStr = "0";
            ResolutionStr = "";
            info.setText(info.getText()+id.toUpperCase()+" Axis: Please enter your mount configuration." + "\n");
            info.scroll(1);
          } else {
            double distnWidth = 32.0;
            println("Using Microsteps: "+getMicrostepMode());
            long aVal = (long)Math.ceil(gearRatio*wormRatio*(360.0/angle)*getMicrostepMode());
            long sVal = (long)Math.round((double)aVal/wormRatio);
            double maxIVal = (86164.0905*8000000.0)/((double)aVal*480.0);
            long IVal = 1200;
            long bVal;
            double minError = 1000;
            int minIVal = Math.min(MIN_IVAL,(int)Math.floor(maxIVal)); //if the maximum possible is smaller than our soft limit, change the minimum.
            minimumIVal[Arrays.asList(axisIDStrings).indexOf(id)] = minIVal;
            boolean wasSuccessful = false;
            for (double testIVal = minIVal; testIVal <= maxIVal; testIVal++){
              double floatAVal = (double)aVal;
              double actualBVal = (testIVal*floatAVal/86164.0905);
              double roundedBVal = Math.round(actualBVal);
              if (actualBVal == 0.0) {
                continue;
              }
              double actualRate = 8000000.0 / actualBVal;
              double rate = Math.floor(8000000.0 / roundedBVal);
              double remainder = 8000000 % ((long)roundedBVal);
              remainder = Math.round(remainder * distnWidth / roundedBVal);
              double averageRate = rate + remainder/distnWidth;
              double error = 86164.0905*averageRate/actualRate;
              error = Math.abs(86164.0905 - error);
              if (error < minError){
                minError = error;
                IVal = (long)testIVal;
                wasSuccessful = true;
              }
            }
            println("Minimised Error: "+String.format("%.5f",minError)+" seconds/sidereal day");
            if (maxIVal < MIN_IVAL) {
              info.setText(info.getText()+id.toUpperCase()+": WARNING IVal small, non-sidereal speed accuracy lower" + "\n");
              info.scroll(1);
            }
            if (!wasSuccessful) {
              IVal = minIVal;
              IValStr = String.valueOf(IVal);
              aValStr = "Error";
              bValStr = "";
              sValStr = "";
              ResolutionStr = "";
              info.setText(info.getText()+id.toUpperCase()+": ERROR! Could not find solution. Try reducing Steps/rev. :(" + "\n");
              info.scroll(1);
            } else if (IVal < ABSOLUTE_MIN_IVAL) {
              IValStr = String.valueOf(IVal);
              aValStr = "Error";
              bValStr = "";
              sValStr = "";
              ResolutionStr = "";
              info.setText(info.getText()+id.toUpperCase()+": ERROR! IVal must be >"+ABSOLUTE_MIN_IVAL+". Try reducing Steps/rev. :(" + "\n");
              info.scroll(1);
            } else if (IVal > 1200) {
              IValStr = String.valueOf(IVal);
              aValStr = "Error";
              bValStr = "";
              sValStr = "";
              ResolutionStr = "";
              info.setText(info.getText()+id.toUpperCase()+": ERROR! IVal must be <1200. Try larger uStep setting. :(" + "\n");
              info.scroll(1);
            } else {
              bVal = (long)Math.round((double)IVal*(double)aVal/86164.0905);
              aValStr = String.valueOf(aVal);
              bValStr = String.valueOf(bVal);
              sValStr = String.valueOf(sVal);
              IValStr = String.valueOf(IVal);
              double resolutionVal = 1296000.0/aVal;
              ResolutionStr = String.format("%.3f", resolutionVal);
              info.setText(info.getText()+id.toUpperCase()+" Axis: Combination generated " + ((maxIVal < MIN_IVAL) ? "with warning! :S" : "successfully! :D") + "\n");
              info.scroll(1);
              
              String gotoMult = calculateGotoMultiplier(IValStr,currentScreen.getField(id+"GotoField").getText(),canJumpToHighspeed(),minIVal);
              String gotoFactor = calculateGotoFactor(IValStr,gotoMult,canJumpToHighspeed(),minIVal);
              println(id.toUpperCase() + " Goto Mult=" + gotoMult);
              println(id.toUpperCase() + " Goto Fact=" + gotoFactor);
              if (gotoFactor == null){
                //Invalid, so default to the minimum value
                gotoFactor = getMinimumGotoFactor(IValStr,canJumpToHighspeed(),minIVal);
                if (gotoFactor == null){ //This will never ever happen!
                  gotoFactor = "IVal Err"; //Still invalid, probably and IVal error, so clear the field.
                }
              }
              currentScreen.getField(id+"GotoField").setText(gotoFactor);
              
              //Create acceleration table
              final int ACCEL_TABLE_LEN = 64;
              final int MAX_REPEATS = 85;
                          
              short[] testSpeeds = new short[ACCEL_TABLE_LEN];
              byte[] testRepeats = new byte[ACCEL_TABLE_LEN];
              boolean success = generateAccelerationTable((short)IVal, (int)bVal, ACCEL_TABLE_LEN, MAX_REPEATS, testSpeeds, testRepeats, id.toUpperCase());
              if (!success){
                info.setText(info.getText() + id.toUpperCase() + " Axis: Failed to calculate Acceleration Table, Cannot Continue." + "\n");
              }
            }
          }
          if (currentScreen.getField("raGuideRateField").getText() == "") {
            currentScreen.getField("raGuideRateField").setText("0.25");
          } else {
            try {
              double st4RateFactor = Double.parseDouble(currentScreen.getField("raGuideRateField").getText());
              if (st4RateFactor > 0.95) {
                currentScreen.getField("raGuideRateField").setText("0.95");
              } else if (st4RateFactor < 0.05) {
                currentScreen.getField("raGuideRateField").setText("0.05");
              } else {
                st4RateFactor = ((double)Math.round(st4RateFactor * 20)) / 20.0; 
                currentScreen.getField("raGuideRateField").setText(String.format( "%.2f", st4RateFactor ));
              }
            } catch (NumberFormatException e) {
              currentScreen.getField("raGuideRateField").setText("0.25");
            }
          }
          
          currentScreen.getField(id+"aValField").setText(aValStr);
          currentScreen.getField(id+"bValField").setText(bValStr);
          currentScreen.getField(id+"sValField").setText(sValStr);
          currentScreen.getField(id+"IValField").setText(IValStr);
          currentScreen.getField(id+"ResolutionField").setText(ResolutionStr);
          if (aValStr.equals("0") || sValStr.equals("0") || bValStr.equals("0") || IValStr.equals("0")) {
            int which = (id.equals("ra") ? 0 : 1);
            screenStatus = (screenStatus & ~(1 << which));
          } else {
            int which = (id.equals("ra") ? 0 : 1);
            screenStatus = (screenStatus | (1 << which));
          }
          Button button = currentScreen.getButton("nextButton");
          if (screenStatus == 3) {
            button.unlock();
            button.setCaptionLabel("Done");
          } else {
            button.lock();
            button.setCaptionLabel("");
          }
        } else if (name.substring(2).equals("axisReverseField")) {
          Button reverseButton = currentScreen.getButton(name);
          if (reverseButton.isOn()){
            reverseButton.setCaptionLabel("Backward");
          } else {
            reverseButton.setCaptionLabel("Forward");
          }
        } else if (name.substring(2).equals("advancedHCEnableField")) {
          Button advancedHCButton = currentScreen.getButton(name);
          if (advancedHCButton.isOn()) {
            advancedHCButton.setCaptionLabel("Enabled");
          } else {
            advancedHCButton.setCaptionLabel("Disabled");
          }
        } else if (name.substring(2).equals("gearchangeEnableField")) {
          Button gearChangeButton = currentScreen.getButton(name);
          if (gearChangeButton.isOn()){
            gearChangeButton.setCaptionLabel("Enabled");
          } else {
            gearChangeButton.setCaptionLabel("Disabled");
          }
          int usteps = getMicrostepMode();
          info.setText(info.getText()+"Step Modes: Slow = "+usteps+(usteps>1?" usteps":" ustep")+"/stp");
          if ((usteps >=8) && gearChangeButton.isOn()) {
            usteps /= 8;
          }
          info.setText(info.getText()+", Fast = "+usteps+(usteps>1?" usteps":" ustep")+"/step\n");
          info.scroll(1);
        } else if (name.equals("nextButton")) {
          if (testRun) {
            setDisplay(GUI_BEGIN); //If we were only testing, return to the beginning
            execute.resetStatus(); //finished with the thread, so clear.
            info.setText("");
            screenStatus = 0;
            println("------ Configuration Complete ------");
          } else if (screenStatus == 3) { 
            String[] id = axisIDStrings;
            String[] gotoMultiplier = new String[2];
            boolean success = true;
            for (int i = 0; i < 2; i++){
              String gotoMult = calculateGotoMultiplier(currentScreen.getField(id[i]+"IValField").getText(),currentScreen.getField(id[i]+"GotoField").getText(),canJumpToHighspeed(),minimumIVal[i]);
              String gotoFact = calculateGotoFactor(currentScreen.getField(id[i]+"IValField").getText(),gotoMult,canJumpToHighspeed(),minimumIVal[i]);
              println(id[i].toUpperCase() + " Goto Mult=" + gotoMult);
              println(id[i].toUpperCase() + " Goto Fact=" + gotoFact);
              if ((gotoMult == null) || (gotoFact == null)){
                info.setText(info.getText()+id[i].toUpperCase()+" Axis: Invalid Goto Speed, Cannot Continue." + "\n");
                success = false;
                break;
              }
              
              gotoMultiplier[i] = gotoMult;
              currentScreen.getField(id[i]+"GotoField").setText(gotoFact);
            }
            if (!success){
              break; //can't continue.
            }
            
            //Create acceleration table
            final int ACCEL_TABLE_LEN = 64;
            final int MAX_REPEATS = 85;
                        
            short[] raSpeeds = new short[ACCEL_TABLE_LEN];
            byte[] raRepeats = new byte[ACCEL_TABLE_LEN];
            short[] dcSpeeds = new short[ACCEL_TABLE_LEN];
            byte[] dcRepeats = new byte[ACCEL_TABLE_LEN];
            
            short raIVal = (short)Integer.parseInt(currentScreen.getField("raIValField").getText());
            short dcIVal = (short)Integer.parseInt(currentScreen.getField("dcIValField").getText());
            int rabVal = Integer.parseInt(currentScreen.getField("rabValField").getText());
            int dcbVal = Integer.parseInt(currentScreen.getField("dcbValField").getText());
            
            success = generateAccelerationTable(raIVal, rabVal, ACCEL_TABLE_LEN, MAX_REPEATS, raSpeeds, raRepeats, "RA");
            if (!success){
              info.setText(info.getText()+"RA Axis: Failed to calculate Acceleration Table, Cannot Continue." + "\n");
              break; //can't continue.
            }
            success = generateAccelerationTable(dcIVal, dcbVal, ACCEL_TABLE_LEN, MAX_REPEATS, dcSpeeds, dcRepeats, "DC");
            if (!success){
              info.setText(info.getText()+"DC Axis: Failed to calculate Acceleration Table, Cannot Continue." + "\n");
              break; //can't continue.
            }
            //Calculate ST4 rate 
            double st4RateFactor;
            try {
              st4RateFactor = Double.parseDouble(currentScreen.getField("raGuideRateField").getText());
              if (st4RateFactor > 0.95) {
                st4RateFactor = 0.95;
              } else if (st4RateFactor < 0.05) {
                st4RateFactor = 0.05;
              }
            } catch (NumberFormatException e) {
              st4RateFactor = 0.25;
            }
            long st4Rate = Math.round(st4RateFactor * 20);
            //Create array of main programming commands
            String[] encodedCommands = new String[] {
              ":A1"+ SyntaString.syntaEncoder(currentScreen.getField("raaValField").getText(), SyntaString.argIsLong, SyntaString.encode), 
              ":B1"+ SyntaString.syntaEncoder(currentScreen.getField("rabValField").getText(), SyntaString.argIsLong, SyntaString.encode), 
              ":S1"+ SyntaString.syntaEncoder(currentScreen.getField("rasValField").getText(), SyntaString.argIsLong, SyntaString.encode),  
              ":N1"+ SyntaString.syntaEncoder(currentScreen.getField("raIValField").getText(), SyntaString.argIsLong, SyntaString.encode),  
              ":C1"+ (currentScreen.getButton("raaxisReverseField").isOn() ? "1" : "0"), 
              ":A2"+ SyntaString.syntaEncoder(currentScreen.getField("dcaValField").getText(), SyntaString.argIsLong, SyntaString.encode), 
              ":B2"+ SyntaString.syntaEncoder(currentScreen.getField("dcbValField").getText(), SyntaString.argIsLong, SyntaString.encode), 
              ":S2"+ SyntaString.syntaEncoder(currentScreen.getField("dcsValField").getText(), SyntaString.argIsLong, SyntaString.encode), 
              ":N2"+ SyntaString.syntaEncoder(currentScreen.getField("dcIValField").getText(), SyntaString.argIsLong, SyntaString.encode), 
              ":C2"+ (currentScreen.getButton("dcaxisReverseField").isOn() ? "1" : "0"), 
              ":D1"+ SyntaString.syntaEncoder(""+(Integer)currentScreen.getScrollableListItem("radriverVersionField"), SyntaString.argIsByte, SyntaString.encode), 
              ":D2"+ SyntaString.syntaEncoder(""+(Integer)currentScreen.getScrollableListItem("ramicrostepEnableField"), SyntaString.argIsByte, SyntaString.encode), 
              ":Q1"+ SyntaString.syntaEncoder(""+(currentScreen.getButton("raadvancedHCEnableField").isOn() ? "1" : "0"), SyntaString.argIsByte, SyntaString.encode),
              ":Q2"+ SyntaString.syntaEncoder(""+(currentScreen.getButton("ragearchangeEnableField").isOn() ? "0" : "1"), SyntaString.argIsByte, SyntaString.encode), 
              ":R1"+ SyntaString.syntaEncoder(""+st4Rate, SyntaString.argIsLong, SyntaString.encode),
              ":Z1"+ SyntaString.syntaEncoder(gotoMultiplier[0], SyntaString.argIsByte, SyntaString.encode),
              ":Z2"+ SyntaString.syntaEncoder(gotoMultiplier[1], SyntaString.argIsByte, SyntaString.encode), 
              ":Y1"+ SyntaString.syntaEncoder("0", SyntaString.argIsByte, SyntaString.encode),
              ":Y2"+ SyntaString.syntaEncoder("0", SyntaString.argIsByte, SyntaString.encode)
            };
            
            //Create extraArgs array with enough space for main commands and accel table.
            final int NUM_COMMANDS = 17;
            extraArgs = new String[NUM_COMMANDS+2*ACCEL_TABLE_LEN+2];
            //Initialise it, starting with the main commands.
            for (int i = 0; i < NUM_COMMANDS; i++) {
              extraArgs[i] = encodedCommands[i];
            }
            //Next add the accel table
            for (int i = 0; i < ACCEL_TABLE_LEN; i++) {
              int raEncodedTable = (((int)raRepeats[i]) << 16) + (int)raSpeeds[i];
              int dcEncodedTable = (((int)dcRepeats[i]) << 16) + (int)dcSpeeds[i];
              extraArgs[NUM_COMMANDS + 2*i + 0] = ":X1"+ SyntaString.syntaEncoder(""+raEncodedTable, SyntaString.argIsLong, SyntaString.encode);
              extraArgs[NUM_COMMANDS + 2*i + 1] = ":X2"+ SyntaString.syntaEncoder(""+dcEncodedTable, SyntaString.argIsLong, SyntaString.encode);
            }
            //Finish up the array with the store commands.
            extraArgs[NUM_COMMANDS+2*ACCEL_TABLE_LEN+0] = ":T1"; //store to EEPROM.
            extraArgs[NUM_COMMANDS+2*ACCEL_TABLE_LEN+1] = ":O10"; //reset the uC.
            
            setDisplay(GUI_STORE_CONFIG);
            execute.resetStatus(); //finished with the thread, so clear.
            info.setText("");
            println("------ Configuration Complete ------");
          } else {
            break;
          }
        }
        break;

      case GUI_STORE_CONFIG:
        if (name.equals("nextButton")) {

          if (screenStatus == 1) {
            setDisplay(GUI_ENTER_CONFIG); //back to previous screen for retry
            screenStatus = 3;
            info.setColorValue(color(#ffffff));
          } else if (screenStatus == 2) {
            if (isWindows()) {
              setDisplay(GUI_LOAD_EQMOD);
            } else if (isUnix()) {
              setDisplay(GUI_FINISHED);
            }
            screenStatus = 0;
          } else {
            break;
          }

          execute.resetStatus(); //finished with the thread, so clear.
        }
        break;

      case GUI_BURN_ASTROEQ:
        if (name.equals("nextButton")) {

          if (screenStatus == 3) {
            setDisplay(GUI_BEGIN); //return to menu.
          } else if (screenStatus == 4) {
            setDisplay(GUI_EEPROM_READ);
          } else if (screenStatus != 5) {
            break;
          }

          execute.resetStatus(); //finished with the thread, so clear.
          
          if (screenStatus == 5) {
            screenStatus = 1;
          } else {
            screenStatus = 0;
          }
          info.setText("");
        }
        break;
      case GUI_LOAD_EQMOD:
        if (name.equals("nextButton")) {
          String finishText;
          if (screenStatus == 3) {
            finishText = "Mount Programmed, Couldn't Test With EQMOD";
          } else if (screenStatus == 4) {
            finishText = "                       AstroEQ Setup Failed!!";
          } else if (screenStatus == 2) {
            finishText = "       AstroEQ Setup Completed Successfully!";
          } else  {
            break;
          }
          
          if (screenStatus == 2) {
            execute   = new ExecutableInterface(this.parent);
            executeThread = new Thread(execute);
            eqmodScript("EQMODQuit.vbs");
            
            while(execute.status().isRunning());
          }
          execute.resetStatus(); //finished with the thread, so clear.
          
          screenStatus = 0;
          
          setDisplay(GUI_FINISHED);
          
          Textlabel finLabel = groups.get(groupNames[currentGroupIndex]).getTextLabel("finLabel");
          //println(currentScreen);
          //println(finLabel);
          if (finLabel != null) {
            finLabel.setValue(finishText);
          }
          info.setText("");
        }
        break;

      case GUI_FINISHED:
        if (name.equals("finButton")) {
          exit();
        } else if (name.equals("returnButton")){
          screenStatus = 0;
          setDisplay(GUI_BEGIN);
          info.setText("");
          execute.resetStatus(); //finished with the thread, so clear.
        }
        break;

      default:
        break;
      }
    }
    //otherwise return whether or not we own the button.
    return found;
  }



//
//
//   Update Display
//
//

  public void updateDisplay(TaskStatus execStatus) {

    GUIScreen currentScreen = groups.get(groupNames[currentGroupIndex]);
    Textlabel message;
    Slider slider;
    Button next;
    
    fill(BORDER_COLOUR);
    noStroke();
    rect(0,dimensionMaker.bottom()-4,dimensionMaker.width(),4);
    rect(0,dimensionMaker.top(),dimensionMaker.width(),TEXTBAR_HEIGHT);
    
      
    switch (currentGroupIndex) {
    case GUI_BEGIN:
      if (screenStatus == 0) {
        //info.setPosition(infoDim.x(),infoDim.y());
        //info.setHeight(infoDim.height());
        screenStatus = 1;
      }
      break;
    case GUI_CHECK_VERSION:
      if (screenStatus > 1) {
        break; //screen is done with.
      }
      message = currentScreen.getTextLabel("messageBox");
      slider = currentScreen.getSlider("busyIndicator");
      next = currentScreen.getButton("nextButton");
      if (!execStatus.isRunning() && execStatus.isComplete() && execStatus.isError()) {
        if (slider != null) {
          slider.setValue(0);
        }
        //println(currentScreen);
        //println(next);
        int errorCode = execStatus.errorCode();
        String buttonLabel;
        String errorMessage;
        if (errorCode == 1) {
          errorMessage = "Found newer firmware on server!";
          buttonLabel = "Next";
          screenStatus = 2;
        } else if (errorCode == 3) {
          errorMessage = "Please use the new AstroEQ Utility!";
          buttonLabel = "Exit";
          link("http://www.astroeq.co.uk/downloads.php");//, "_new");
          screenStatus = 6;
        } else {
          errorMessage = "An Error Occurred!!";
          if (screenStatus == 0) {
            buttonLabel = "Retry";
            screenStatus = 5;
          } 
          else {
            screenStatus = 4;
            buttonLabel = "Skip";
          }
        }
        if (message != null) {
          message.setValue(errorMessage);
        }
        if (next != null) {
          next.unlock();
          next.setCaptionLabel(buttonLabel);
        }
        println(errorMessage);
      } else if (((!execStatus.isRunning() && execStatus.isComplete()) || (__TESTING__ != 0))) {
        if (slider != null) {
          slider.setValue(0);
        }
        if (next != null) {
          next.unlock();
          next.setCaptionLabel("Next");
        }
        if (message != null) {
          message.setValue("Check Complete. No new firmware version.");
        }
        screenStatus = 3;
      } else if (!execStatus.isRunning()) {
        if (slider != null) {
          slider.setValue(0);
        }
          
        execute   = new FileUpdateCheck(parent);
        executeThread = new Thread(execute);
        executeTask(new String[1]);  
        extraArgs = null;
        if (message != null) {
          message.setValue("Checking AstroEQ Github for new versions...");
        }
        if (next != null) {
          next.setMouseOver(false);
          next.setMousePressed(false);
          next.lock();
          next.setCaptionLabel("");
        }
      } else if (execStatus.isRunning()) {
        if (slider != null) {
          slider.setValue((float)((slider.getValue()+2 )%100));
        }
      }
      break;
      
    case GUI_FETCH_FIRMWARE:
      if (screenStatus > 1) {
        break; //screen is done with.
      }
      message = currentScreen.getTextLabel("messageBox");
      slider = currentScreen.getSlider("busyIndicator");
      next = currentScreen.getButton("nextButton");
      
      if (!execStatus.isRunning() && execStatus.isComplete() && execStatus.isError()) {
        if (slider != null) {
          slider.setValue(0);
        }
        //println(currentScreen);
        //println(next);
        int errorCode = execStatus.errorCode();
        String buttonLabel;
        String errorMessage;
        if (errorCode == 1) {
          errorMessage = "No new firmware to download!";
          buttonLabel = "Next";
          screenStatus = 2;
        } else {
          errorMessage = "Could not find file on server!!";
          if (screenStatus == 0) {
            buttonLabel = "Retry";
            screenStatus = 5;
          } 
          else {
            screenStatus = 4;
            buttonLabel = "Skip";
          }
        }
        if (message != null) {
          message.setValue(errorMessage);
        }
        if (next != null) {
          next.unlock();
          next.setCaptionLabel(buttonLabel);
        }
        println(errorMessage);
      } else if (((!execStatus.isRunning() && execStatus.isComplete()) || (__TESTING__ != 0))) {
        if (slider != null) {
          slider.setValue(0);
        }
        if (next != null) {
          next.unlock();
          next.setCaptionLabel("Next");
        }
        if (message != null) {
          message.setValue("New firmware downloaded successfully.");
        }
        screenStatus = 3;
      } else if (!execStatus.isRunning()) {
        if (slider != null) {
          slider.setValue(0);
        }
          
        execute   = new NewFirmwareFetcher(parent);
        executeThread = new Thread(execute);
        executeTask(extraArgs);  
        extraArgs = null;
        if (message != null) {
          message.setValue("Fetching new firmware from AstroEQ Github...");
        }
        if (next != null) {
          next.setMouseOver(false);
          next.setMousePressed(false);
          next.lock();
          next.setCaptionLabel("");
        }
      } else if (execStatus.isRunning()) {
        if (slider != null) {
          slider.setValue((float)((slider.getValue()+2 )%100));
        }
      }
      break;
      
    case GUI_BURN_UTILITY:
      if (screenStatus != 0) {
        break; //screen is done with.
      }
      message = currentScreen.getTextLabel("messageBox");
      slider = currentScreen.getSlider("busyIndicator");
      next = currentScreen.getButton("nextButton");
      if ((curPort != null) || (__TESTING__ != 0)) {
        if (!execStatus.isRunning() && execStatus.isComplete() && execStatus.isError()) {
          if (slider != null) {
            slider.setValue(0);
          }
          //println(currentScreen);
          //println(next);
          if (next != null) {
            next.unlock();
            next.setCaptionLabel("Quit");
          }
          if (message != null) {
            message.setValue("Upload Failed!! Check log for details");
          }
          screenStatus = 1;
        } 
        else if ((!execStatus.isRunning() && execStatus.isComplete()) || (__TESTING__ != 0)) {
          if (slider != null) {
            slider.setValue(0);
          }
          if (next != null) {
            next.unlock();
            next.setCaptionLabel("Next");
          }
          if (message != null) {
            message.setValue("Upload Succeeded!!");
          }
          screenStatus = 2;
        } 
        else if (!execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue(0);
          }
          println(curFile+"EEPROMReader");
          
          execute   = new ExecutableInterface(parent);
          executeThread = new Thread(execute);
          avrdudeRun(curFile+"EEPROMReader", curPort, boardVersion);
          if (message != null) {
            message.setValue("File Upload in Progress...");
          }
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        } 
        else if (execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue((float)((slider.getValue()+2 )%100));
          }
        }
      } 
      else {
        if (message != null) {
          message.setValue("Please Select COM port!");
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        }
      }
      break;

    case GUI_EEPROM_READ:
      if (screenStatus > 1) {
        break; //screen is done with.
      }
      message = currentScreen.getTextLabel("messageBox");
      slider = currentScreen.getSlider("busyIndicator");
      next = currentScreen.getButton("nextButton");
      if ((curPort != null) || (__TESTING__ != 0)) {
        if (!execStatus.isRunning() && execStatus.isComplete() && execStatus.isError()) {
          if (slider != null) {
            slider.setValue(0);
          }
          //println(currentScreen);
          //println(next);
          int errorCode = execStatus.errorCode();
          String buttonLabel;
          String errorMessage;
          if (errorCode == 1) {
            errorMessage = "Read Suceeded. EEPROM Blank or Corrupt!";
            buttonLabel = "Next";
            screenStatus = 2;
          } 
          else if (errorCode == 2) {
            errorMessage = "Read Error!! Lost communication.";
            if (screenStatus == 0) {
              buttonLabel = "Retry";
              screenStatus = 5;
            } 
            else {
              screenStatus = 3;
              buttonLabel = "Quit";
            }
          } 
          else {
            errorMessage = "Read Failed!! Connection unavailable.";
            screenStatus = 3;
            buttonLabel = "Quit";
          }
          if (message != null) {
            message.setValue(errorMessage);
          }
          if (next != null) {
            next.unlock();
            next.setCaptionLabel(buttonLabel);
          }
          closeSerialPort();
          println(errorMessage);
        } 
        else if ((!execStatus.isRunning() && execStatus.isComplete()) || (__TESTING__ != 0)) {
          if (slider != null) {
            slider.setValue(0);
          }
          if (next != null) {
            next.unlock();
            next.setCaptionLabel("Next");
          }
          if (message != null) {
            message.setValue("Read Succeeded. Loaded configuration.");
          }
          closeSerialPort();
          screenStatus = 4;
        } 
        else if (!execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue(0);
          }
          openSerialPort();
          execute   = new EEPROMInterface(parent,inQueue,outQueue);
          executeThread = new Thread(execute);
          extraArgs = new String[] {
            ":T1", //check EEPROM.
            ":a1",  //aVal
            ":b1",  //bVal
            ":s1",  //sVal
            ":n1",  //IVal at sidereal
            ":c1",  //reverse axis
            ":d1",  //driver board version
            ":a2",  //aVal
            ":b2",  //bVal
            ":s2",  //sVal
            ":n2",  //IVal
            ":c2",  //reverse axis
            ":d2",  //step mode
            ":q1",  //advanced hc detection enabled
            ":q2",  //gear change disabled.
            ":r1",  //ST4 rate.
            ":z1",  //Goto Speed factor.
            ":z2"
          };
          eepromProgrammerRun(EEPROMProgrammer.ReadOrRun, EEPROMProgrammer.NoRepair, extraArgs);
          extraArgs = null;
          if (message != null) {
            message.setValue("EEPROM Read in Progress...");
          }
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        } 
        else if (execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue((float)((slider.getValue()+2 )%100));
          }
          processSerialData();
        }
      } 
      else {
        if (message != null) {
          message.setValue("Please Select COM port!");
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        }
      }
      break;

    case GUI_EEPROM_RECOVER:
      if (screenStatus != 0) {
        break; //screen is done with.
      }
      message = currentScreen.getTextLabel("messageBox");
      slider = currentScreen.getSlider("busyIndicator");
      next = currentScreen.getButton("nextButton");
      if ((curPort != null) || (__TESTING__ != 0)) {
        if (!execStatus.isRunning() && execStatus.isComplete() && execStatus.isError()) {
          if (slider != null) {
            slider.setValue(0);
          }
          //println(currentScreen);
          //println(next);
          if (next != null) {
            next.unlock();
            next.setCaptionLabel("Quit");
          }
          if (message != null) {
            message.setValue("Repair Failed!! Connection unavailable.");
          }
          closeSerialPort();
          screenStatus = 1;
        } 
        else if (((!execStatus.isRunning() && execStatus.isComplete()) || (__TESTING__ != 0))) {
          if (slider != null) {
            slider.setValue(0);
          }
          if (next != null) {
            next.unlock();
            next.setCaptionLabel("Next");
          }
          if (message != null) {
            message.setValue("EEPROM Repaired. AstroEQ Ready for Configuration.");
          }
          closeSerialPort();
          screenStatus = 2;
        } 
        else if (!execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue(0);
          }
          openSerialPort();
          execute   = new EEPROMInterface(parent,inQueue,outQueue);
          executeThread = new Thread(execute);
          eepromProgrammerRun(EEPROMProgrammer.Store, EEPROMProgrammer.Repair, new String[]{":T1"}); //:=command, T=Proceed, 1=ignored axis.
          extraArgs = null;
          if (message != null) {
            message.setValue("EEPROM Repair in Progress...");
          }
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        } 
        else if (execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue((float)((slider.getValue()+2 )%100));
          }
          processSerialData();
        }
      } 
      else {
        if (message != null) {
          message.setValue("Please Select COM port!");
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        }
      }
      break;

    case GUI_ENTER_CONFIG:
      //info.hide();
      fill(BORDER_COLOUR);
      noStroke();
      rect(dimensionMaker.centre()-1, dimensionMaker.y()+2*TEXTBAR_HEIGHT+2, 2, dimensionMaker.height()-2*TEXTBAR_HEIGHT-2); //central divider
      rect(dimensionMaker.left(), dimensionMaker.y()+2*TEXTBAR_HEIGHT+2, dimensionMaker.width(), 4); //top divider
      rect(dimensionMaker.left(), dimensionMaker.y()+5*TEXTBAR_HEIGHT+8, dimensionMaker.width(), 2); //middle divider

      //fill(#000000);
      //rect(0, dimensionMaker.y()+dimensionMaker.height()+1*TEXTBAR_HEIGHT+8, dimensionMaker.width(), height); //bottom divider
      
      fill(10,21,75);
      rect(0,529,516,2);
  
      fill(#ffffff);
      rect(4, dimensionMaker.y()+6*TEXTBAR_HEIGHT+9, 144, 2);
      rect(dimensionMaker.centre()+4, dimensionMaker.y()+6*TEXTBAR_HEIGHT+9, 113, 2);
      rect(4, dimensionMaker.y()+12*TEXTBAR_HEIGHT+1, 158, 2);
      rect(dimensionMaker.centre()+4, dimensionMaker.y()+12*TEXTBAR_HEIGHT+1, 164, 2);
      
      //rect(4, dimensionMaker.y()+dimensionMaker.height()+TEXTBAR_HEIGHT+3, 162, 2);
      stroke(#000000);
      
      //info.setPosition(infoDim.x(),dimensionMaker.y()+dimensionMaker.height()+1*TEXTBAR_HEIGHT+8);
      //info.setHeight(height - (dimensionMaker.y()+dimensionMaker.height()+1*TEXTBAR_HEIGHT+8));
      
      if (extraArgs != null) {
        //info.setText("");
        //String[] readBack = Arrays.copyOfRange(extraArgs, 1, extraArgs.length);
        for (String readBack : extraArgs) {
          //println(readBack);
          String[] id = readBack.split("0|1|2|3|4|5|6|7|8|9");
          if ((id == null) || (id.length < 1)) {
            id = new String[] {
              "grr"
            };
          }
          if (id[0].charAt(0)==':') continue;
          String name = id[0]+"Field";
          println(id[0].substring(0,2).toUpperCase()+" "+id[0].substring(2)+" is set to: "+readBack.substring(id[0].length()));
          Textfield field = currentScreen.getField(name);
          if ((field != null) && !name.substring(2).equals("GotoField")) {
            field.setText(readBack.substring(id[0].length()));
            if(name.substring(2).equals("aValField")){
              try{
                double resolutionVal = 1296000.0/Double.parseDouble(readBack.substring(id[0].length()));
                currentScreen.getField(id[0].substring(0,2)+"ResolutionField").setText(String.format("%.3f", resolutionVal));
              } catch (Exception e){
                currentScreen.getField(id[0].substring(0,2)+"ResolutionField").setText("");
              }
            }
          } 
          if (name.substring(2).equals("axisReverseField")) {
            Button reverseButton = currentScreen.getButton(name);
            if (readBack.substring(id[0].length()).equals("1")){
              reverseButton.setOn();
              reverseButton.setCaptionLabel("Backward");
            } else {
              reverseButton.setOff();
              reverseButton.setCaptionLabel("Forward");
            }
          } else if (name.substring(2).equals("driverVersionField")) {
            ScrollableList versionDropdown = currentScreen.getDropdown(name);
            ScrollableList microstepDropdown = currentScreen.getDropdown("ramicrostepEnableField");
            if ((versionDropdown != null) && (microstepDropdown != null)){    
              currentScreen.selectScrollableListItem(versionDropdown,Integer.parseInt(readBack.substring(id[0].length())));
              if ((Integer)currentScreen.getScrollableListItem(versionDropdown) != DRIVER_A498x){
                microstepDropdown.removeItem("32 uStep");//ensure there aren't duplicates
                microstepDropdown.addItem("32 uStep", 32);
              } else {
                if ((Integer)currentScreen.getScrollableListItem(microstepDropdown)==32){
                  currentScreen.selectScrollableListItem(microstepDropdown,16);
                }
                microstepDropdown.removeItem("32 uStep");
              }
            }
          } else if (name.substring(2).equals("microstepEnableField")) {
            ScrollableList list = currentScreen.getDropdown("ramicrostepEnableField");
            Integer value = (Integer)currentScreen.getScrollableListItem(list,readBack.substring(id[0].length()) + " uStep","value");
            if (value != null) {
              //Found the required item
              currentScreen.selectScrollableListItem(list,value);
              println(readBack.substring(id[0].length())+" maps to value: "+value);
              Button gearchangeButton = currentScreen.getButton("ragearchangeEnableField");
              if (value >=8) {
                gearchangeButton.unlock(); //Allow changing.
              } else {
                gearchangeButton.lock(); //Prevent changing.
              }
            } else {
              //Didn't find the item
              currentScreen.selectScrollableListItem(list,1);
              println(readBack.substring(id[0].length())+" maps to nothing!");   
            }
          } else if (name.substring(2).equals("GotoField")) {
            String gotoFactor = calculateGotoFactor(currentScreen.getField(name.substring(0,2)+"IValField").getText(),readBack.substring(id[0].length()),canJumpToHighspeed(),minimumIVal[Arrays.asList(axisIDStrings).indexOf(id[0].substring(0,2))]);
            if (gotoFactor == null) {
              gotoFactor = "";
            }
            if (field != null) {
              field.setText(gotoFactor);
            }
          } else if (name.substring(2).equals("advancedHCEnableField")) {
            Button advancedHCButton;
            if ((advancedHCButton = currentScreen.getButton(name)) != null) {
              if (readBack.substring(id[0].length()).equals("1")){
                advancedHCButton.setOn();
                advancedHCButton.setCaptionLabel("Enabled");
              } else {
                advancedHCButton.setOff();
                advancedHCButton.setCaptionLabel("Disabled");
              }
            }
          } else if (name.substring(2).equals("gearchangeEnableField")) {
            Button gearchangeButton;
            if ((gearchangeButton = currentScreen.getButton(name)) != null) {
              Integer usteps = (Integer)currentScreen.getScrollableListItem(currentScreen.getDropdown("ramicrostepEnableField"));
              if (readBack.substring(id[0].length()).equals("0") && (usteps >= 8)){
                gearchangeButton.setOn();
                gearchangeButton.setCaptionLabel("Enabled");
              } else {
                gearchangeButton.setOff();
                gearchangeButton.setCaptionLabel("Disabled");
              }
            }
          } else if (name.substring(2).equals("GuideRateField")) {
            float value = Float.parseFloat(readBack.substring(id[0].length()));
            if (value > 1) {
              //When reading back from EEPROM, the value is actually stored multiplied by 20, so correct it.
              value = value / 20.0;
            }
            currentScreen.getField("raGuideRateField").setText(String.format( "%.2f", value ));
          }
        

          String[] fieldNames = {
            "aValField", "bValField", "sValField", "IValField", "GotoField"
          };
          for (int i = 0; i < 2; i++) {
            String axis = (i == 0) ? "ra" : "dc";
            boolean missingField = false;
            for (String fieldName : fieldNames) {
              String val = currentScreen.getField(axis + fieldName).getText();
              if ((val == null) || (val.length() < 1) || (val.equals("0"))) {
                missingField = true;
                break;
              }
            }
            if (missingField) {
              screenStatus = (screenStatus & ~(1 << i));
            } else {
              screenStatus = (screenStatus | (1 << i));
            }
          }
          Button button = currentScreen.getButton("nextButton");
          if ((screenStatus == 3) || (testRun)) {
            button.unlock();
            button.setCaptionLabel("Done");
          } else {
            button.lock();
            button.setCaptionLabel("");
          }
        }
        extraArgs = null;
      } else {
        Button button = currentScreen.getButton("nextButton");
        if (testRun) {
          button.unlock();
          button.setCaptionLabel("Done");
          screenStatus = 3;
        } else if (screenStatus != 3) {
          button.lock();
          button.setCaptionLabel("");
        }
      }

      break;

    case GUI_STORE_CONFIG:
      if (screenStatus != 0) {
        break; //screen is done with.
      }
      message = currentScreen.getTextLabel("messageBox");
      slider = currentScreen.getSlider("busyIndicator");
      next = currentScreen.getButton("nextButton");
      if ((curPort != null) || (__TESTING__ != 0)) {
        if (!execStatus.isRunning() && execStatus.isComplete() && execStatus.isError()) {
          if (slider != null) {
            slider.setValue(0);
          }
          if (next != null) {
            next.unlock();
            next.setCaptionLabel("Back");
          }
          if (message != null) {
            message.setValue("Write Failed!! Connection unavailable.");
          }
          closeSerialPort();
          screenStatus = 1;
        } 
        else if (((!execStatus.isRunning() && execStatus.isComplete()) || (__TESTING__ != 0))) {
          if (slider != null) {
            slider.setValue(0);
          }
          if (next != null) {
            next.unlock();
            next.setCaptionLabel("Next");
          }
          if (message != null) {
            message.setValue("Configuration Stored Successfully!!");
          }
          closeSerialPort();
          screenStatus = 2;
        } 
        else if (!execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue(0);
          }
          openSerialPort();
          execute   = new EEPROMInterface(parent, inQueue, outQueue);
          executeThread = new Thread(execute);
          eepromProgrammerRun(EEPROMProgrammer.Store, EEPROMProgrammer.NoRepair, extraArgs);
          extraArgs = null;
          if (message != null) {
            message.setValue("Writing Configuration to AstroEQ...");
          }
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        } 
        else if (execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue((float)((slider.getValue()+2 )%100));
          }
          processSerialData();
        }
      } 
      else {
        if (message != null) {
          message.setValue("Please Select COM port!");
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        }
      }
      break;

    case GUI_BURN_ASTROEQ:
      if (screenStatus > 1) {
        break; //screen is done with.
      }
      message = currentScreen.getTextLabel("messageBox");
      slider = currentScreen.getSlider("busyIndicator");
      next = currentScreen.getButton("nextButton");
      if ((curPort != null) || (__TESTING__ != 0)) {
        if (!execStatus.isRunning() && execStatus.isComplete() && execStatus.isError()) {
          if (slider != null) {
            slider.setValue(0);
          }
          //println(currentScreen);
          //println(next);
          //int errorCode = execStatus.errorCode();
          String buttonLabel;
          String errorMessage;

          errorMessage = "Upload Failed!!";
          if (screenStatus == 0) {
            buttonLabel = "Retry";
            screenStatus = 5;
          } 
          else {
            screenStatus = 3;
            buttonLabel = "Quit";
          }
          if (message != null) {
            message.setValue(errorMessage);
          }
          if (next != null) {
            next.unlock();
            next.setCaptionLabel(buttonLabel);
          }
          println(errorMessage);
        } 
        else if (((!execStatus.isRunning() && execStatus.isComplete()) || (__TESTING__ != 0))) {
          if (slider != null) {
            slider.setValue(0);
          }
          if (next != null) {
            next.unlock();
            next.setCaptionLabel("Next");
          }
          if (message != null) {
            message.setValue("AstroEQ Firmware Uploaded Successfully.");
          }
          screenStatus = 4;
        } 
        else if (!execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue(0);
          }
          println(curFile);
            
          execute   = new ExecutableInterface(parent);
          executeThread = new Thread(execute);
          avrdudeRun(curFile, curPort, boardVersion);
          extraArgs = null;
          if (message != null) {
            message.setValue("Uploading AstroEQ Firmware...");
          }
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        } 
        else if (execStatus.isRunning()) {
          if (slider != null) {
            slider.setValue((float)((slider.getValue()+2 )%100));
          }
        }
      } 
      else {
        if (message != null) {
          message.setValue("Please Select COM port!");
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        }
      }
      break;

    case GUI_LOAD_EQMOD:
      if (screenStatus > 1) {
        break; //screen is done with.
      }
      message = currentScreen.getTextLabel("messageBox");
      next = currentScreen.getButton("nextButton");
      if ((curPort != null) || (__TESTING__ != 0)) {
        if (screenStatus == 0) {
          PushbackReader br = null;
          try {
            FileWriter wr = new FileWriter(System.getenv("APPDATA")+"\\EQMOD\\__EQMOD.ini");
            br = new PushbackReader(new FileReader(System.getenv("APPDATA")+"\\EQMOD\\EQMOD.ini"));
            String line = "start";
            int in = 0;
            while (in != -1) {
              StringBuilder builder = new StringBuilder();
              while(true) {
                in = br.read();
                if (in == '\r') {
                  in = br.read(); //see if it is a <CR><LF>
                  if(in != '\n') {
                    br.unread(in); //nope, just <CR>, so put back.
                    in = '\n'; //pretend it was an <LF>
                  }
                }
                if ((in == -1) || (in == '\n')) {
                  //EOF or new line
                  line = builder.toString();
                  break;
                } else {
                  builder.append((char)in);
                }
              }
              if(line != null){
                if(line.length() >= 4){
                  if (line.substring(0, 4).equals("Port")) {
                    line = "Port="+curPort;
                    println(line);
                  } else if (line.substring(0, 4).equals("Baud")) {
                    line = "Baud=9600";
                    println(line);
                  }
                }
                wr.write(line);
              }
              if (in != -1) {
                wr.write(System.getProperty("line.separator"));
              }
              wr.flush();
            }
            println("done");
            br.close();
            wr.close();
          } 
          catch (Exception e) {
            screenStatus = 3;
          } 
          finally {
            FileInputStream sourceStream = null;
            FileChannel source = null;
            FileOutputStream destinationStream = null;
            FileChannel destination = null;
            try {
              PrintWriter writer = new PrintWriter(System.getenv("APPDATA")+"\\EQMOD\\EQMOD.ini");
              writer.print("");
              writer.close();
              sourceStream = new FileInputStream(System.getenv("APPDATA")+"\\EQMOD\\__EQMOD.ini");
              source = sourceStream.getChannel();
              destinationStream = new FileOutputStream(System.getenv("APPDATA")+"\\EQMOD\\EQMOD.ini");
              destination = destinationStream.getChannel();
              destination.transferFrom(source, 0, source.size());
              source.close();
              destination.close();
            } catch (Exception e) {
              screenStatus = 3;
            } finally {
              try {
                if (sourceStream != null) {
                  sourceStream.close();
                }
                if (source != null) {
                  source.close();
                }
                if (destinationStream != null) {
                  destinationStream.close();
                }
                if (destination != null) {
                  destination.close();
                }
                
                File f = new File(System.getenv("APPDATA")+"\\EQMOD\\__EQMOD.ini");
                
                if(f.delete()){
                  println("Deleted");
                }else{
                  println("Failed");
                }
  
                screenStatus = 1;
              } catch (Exception e) {
                screenStatus = 3;
              }
            }
          }
          if(screenStatus == 3) {
            if (message != null) {
              message.setValue("Failed to update EQMOD COM port.");
            }
          }
        } else {
          if (!execStatus.isRunning() && execStatus.isComplete() && execStatus.isError()) {
            //println(currentScreen);
            //println(next);
            if (next != null) {
              next.unlock();
              next.setCaptionLabel("Next");
            }
            int errorCode = execStatus.errorCode();
            String errorMessage;
            if (errorCode == 1) {
              errorMessage = "Failed to open EQMOD! Is it installed?";
              screenStatus = 3;
            } else {
              errorMessage = "EQMOD Couldn't connect to mount!!";
              screenStatus = 4;
            }
            if (message != null) {
              message.setValue(errorMessage);
            }
          } else if (((!execStatus.isRunning() && execStatus.isComplete()) || (__TESTING__ != 0))) {
            if (next != null) {
              next.unlock();
              next.setCaptionLabel("Done");
            }
            if (message != null) {
              message.setValue("EQMOD Loaded. Test Mount. Then Click Done.");
            }
            screenStatus = 2;
          } else if (!execStatus.isRunning()) {
            execute   = new ExecutableInterface(parent);
            executeThread = new Thread(execute);
            eqmodScript("EQMODLoad.vbs");
            extraArgs = null;
            if (message != null) {
              message.setValue("Loading EQMOD for Communication Test...");
            }
            if (next != null) {
              next.setMouseOver(false);
              next.setMousePressed(false);
              next.lock();
              next.setCaptionLabel("");
            }
          }
        } 
      } else {
        if (message != null) {
          message.setValue("Please Select COM port!");
          if (next != null) {
            next.setMouseOver(false);
            next.setMousePressed(false);
            next.lock();
            next.setCaptionLabel("");
          }
        }
      }
      break;

    case GUI_FINISHED:

      break;

    default:
      break;
    }

    refresh();
  }

  public void setHidden(int i) {
    groups.get(groupNames[i]).hideGroup();
  }

  public void setVisible(int i) {
    groups.get(groupNames[i]).showGroup();
  }

  public void refresh() {
    setVisible(currentGroupIndex);
  }

  public void setDisplay(int i) {
    incrementDisplayBy(i - currentGroupIndex);
  }
  
  public String getTextOfFocusTextField() {
    return groups.get(groupNames[currentGroupIndex]).getTextOfFocusTextField();
  }
  
  public void setTextOfFocusTextField(String text){
    groups.get(groupNames[currentGroupIndex]).setTextOfFocusTextField(text);
  }
  
  public void focusNextTextField() {
    groups.get(groupNames[currentGroupIndex]).focusNextTextField();
  }
  
  public void focusPreviousTextField() {
    groups.get(groupNames[currentGroupIndex]).focusPreviousTextField();
  }

  public void incrementDisplayBy(int increment) {
    setHidden(currentGroupIndex);
    currentGroupIndex = (currentGroupIndex + increment) % groups.size();
    setVisible(currentGroupIndex);
    screenStatus = 0;
  }

  private void buildBeginScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());
    Dimensions dimension = new Dimensions(0, 0);
    dimension.setSize(200, 3*ELEMENT_HEIGHT);
    int y = dimensionMaker.height()/2+(ELEMENT_HEIGHT+TEXTBAR_HEIGHT)/2;
    int x = dimensionMaker.width()/2-100;
    dimension.setOrigin(x, y-15*TEXTBAR_HEIGHT/2);
    screen.addButton( control, "configButton", "Configure", wl_font, 1, dimension);
    
    dimension.setOrigin(x, y-9*TEXTBAR_HEIGHT/2);
    screen.addButton( control, "uploadButton", "Update Firmware", wl_font, 1, dimension);
    
    dimension.setOrigin(x, y-3*TEXTBAR_HEIGHT/2);
    screen.addButton( control, "firmwareButton", "Check For New Version", wl_font, 1, dimension);
    
    dimension.setOrigin(x, y+3*TEXTBAR_HEIGHT/2);
    screen.addButton( control, "testButton", "Test Settings", wl_font, 1, dimension);
    
    dimension.setOrigin(x, y+9*TEXTBAR_HEIGHT/2);
    screen.addButton( control, "eqmodButton", "Load EQMOD", wl_font, 1, dimension);
  }

  private void buildEEPROMWriterScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());

    Dimensions dimension = new Dimensions(0, 0);

    //Add message label
    dimension.setSize(100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addTextLabel(control, "messageBox", wl_font, "", color(#ffffff), dimension);

    //Add progress indicator
    dimension.setSize(dimensionMaker.width()-100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) - TEXTBAR_HEIGHT/2);
    Slider busy = screen.addSlider(control, "busyIndicator", 0, 100, 0, dimension);
    busy.lock();
    busy.getValueLabel().setVisible(false);

    //Add next/back button
    dimension.setSize(50, ELEMENT_HEIGHT);
    dimension.setOrigin(dimensionMaker.width()-100, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addButton( control, "nextButton", "", wl_font, 1, dimension).lock();
  }

  private void buildCheckEEPROMScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());

    Dimensions dimension = new Dimensions(0, 0);

    //Add message label
    dimension.setSize(100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addTextLabel(control, "messageBox", wl_font, "", color(#ffffff), dimension);

    //Add progress indicator
    dimension.setSize(dimensionMaker.width()-100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) - TEXTBAR_HEIGHT/2);
    Slider busy = screen.addSlider(control, "busyIndicator", 0, 100, 0, dimension);
    busy.lock();
    busy.getValueLabel().setVisible(false);

    //Add next/back button
    dimension.setSize(50, ELEMENT_HEIGHT);
    dimension.setOrigin(dimensionMaker.width()-100, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addButton( control, "nextButton", "", wl_font, 1, dimension).lock();
  }

  private void buildRecoverEEPROMScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());

    Dimensions dimension = new Dimensions(0, 0);

    //Add message label
    dimension.setSize(100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addTextLabel(control, "messageBox", wl_font, "", color(#ffffff), dimension);

    //Add progress indicator
    dimension.setSize(dimensionMaker.width()-100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) - TEXTBAR_HEIGHT/2);
    Slider busy = screen.addSlider(control, "busyIndicator", 0, 100, 0, dimension);
    busy.lock();
    busy.getValueLabel().setVisible(false);

    //Add next/back button
    dimension.setSize(50, ELEMENT_HEIGHT);
    dimension.setOrigin(dimensionMaker.width()-100, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addButton( control, "nextButton", "", wl_font, 1, dimension).lock();
  }

  private void buildEnterConfigScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());

    String[] id = {
      "ra", "dc"
    };
    axisIDStrings = id;
    String[] title = {
      "Right Ascension Axis:", "Declination Axis:"
    };
    String[] axis = {
      "RA", "Dec"
    };

    final int mainLabelWidth = 100;
    final int borderWidth = 1;
    Dimensions dimensionLabel = new Dimensions(0, 0,                        0, TEXTBAR_HEIGHT+3, 340, ELEMENT_HEIGHT);
    Dimensions dimension      = new Dimensions(0, 0, dimensionLabel.width()+4, TEXTBAR_HEIGHT+3,  24, ELEMENT_HEIGHT);
    
    screen.addTextLabel(control, "messageBox", wl_font, "Enter Mount Configuration Below, or Open Confing:", color(#ffffff), dimensionLabel);

    //Add file buttons
    screen.addButton( control, "loadButton", "Load", wl_font, 1, dimension)
           .setImages(loadImage("Load.png"), loadImage("Load-Over.png"), loadImage("Load-Down.png"));
    
    dimension.shift(dimension.width()+4,0);
    screen.addButton( control, "saveButton", "Save", wl_font, 1, dimension)
           .setImages(loadImage("Save.png"), loadImage("Save-Over.png"), loadImage("Save-Down.png"));
    
    dimension.shift(dimension.width()+4,0);
    screen.addButton( control, "resetButton", "Reset", wl_font, 1, dimension)
           .setImages(loadImage("Reset.png"), loadImage("Reset-Over.png"), loadImage("Reset-Down.png"));
    
    //Add done buttons
    dimension.setWidth(dimensionMaker.right()-6-dimension.right());
    dimension.setX(dimensionMaker.right()-2-dimension.width());
    screen.addButton( control, "nextButton", "Done", wl_font, 1, dimension).lock();
    
    int y = 0;
    final int upperLabelWidth = 160;
    final int lowerLabelWidth = 170;
    
    for (int i = 0; i < 2; i++) {
      y = 5*TEXTBAR_HEIGHT+12;
      dimensionLabel = new Dimensions(i*(dimensionMaker.centre()+1)+borderWidth, borderWidth,               0, y,                                            upperLabelWidth, ELEMENT_HEIGHT);
      dimension      = new Dimensions(i*(dimensionMaker.centre()+1)+borderWidth, borderWidth, upperLabelWidth, y, dimensionMaker.width()/2-upperLabelWidth-2-(2*borderWidth), ELEMENT_HEIGHT);
      screen.addTextLabel(control, id[i]+"Title", wl_font, title[i], color(#ffffff), dimensionLabel);
      screen.addButton( control, id[i]+"axisReverseField", "Forward", wl_font, 1, dimension).setSwitch(true);
      y += TEXTBAR_HEIGHT+4;
      dimension.setY(y);
      dimensionLabel.setY(y);
      screen.addTextLabel(control, id[i]+"Angle", wl_font, "Motor Step Angle   (°):", color(#ffffff), dimensionLabel);
      screen.addField(control, id[i]+"AngleField", ControlP5.FLOAT, wl_font, color(#000000), true, dimension);
      y += TEXTBAR_HEIGHT;
      dimension.setY(y);
      dimensionLabel.setY(y);
      screen.addTextLabel(control, id[i]+"UGear", wl_font, "Motor Gear Ratio  (:1):", color(#ffffff), dimensionLabel);
      screen.addField(control, id[i]+"UGearField", ControlP5.FLOAT, wl_font, color(#000000), true, dimension);
      y += TEXTBAR_HEIGHT;
      dimension.setY(y);
      dimensionLabel.setY(y);
      screen.addTextLabel(control, id[i]+"WGear", wl_font, "Worm Gear Ratio  (:1):", color(#ffffff), dimensionLabel);
      screen.addField(control, id[i]+"WGearField", ControlP5.FLOAT, wl_font, color(#000000), true, dimension);
      y += TEXTBAR_HEIGHT;
      dimension.setY(y);
      dimensionLabel.setY(y);
      screen.addTextLabel(control, id[i]+"Goto", wl_font, "Goto Rate (x Sidereal):", color(#ffffff), dimensionLabel);
      screen.addField(control, id[i]+"GotoField", ControlP5.FLOAT, wl_font, color(#000000), true, dimension);
      
      y += TEXTBAR_HEIGHT+8;
      dimensionLabel = new Dimensions(i*(dimensionMaker.centre()+1)+borderWidth, borderWidth,               0, y,                                            lowerLabelWidth, ELEMENT_HEIGHT);
      dimension =      new Dimensions(i*(dimensionMaker.centre()+1)+borderWidth, borderWidth, lowerLabelWidth, y, dimensionMaker.width()/2-lowerLabelWidth-2-(2*borderWidth), ELEMENT_HEIGHT);
      
      screen.addTextLabel(control, id[i]+"Setup", wl_font, "Calculated "+axis[i]+" Settings:", color(#ffffff), dimensionLabel);
      screen.addButton(control, id[i]+"UpdateButton", "Update", wl_font, 1, dimension);

      y += TEXTBAR_HEIGHT+4;
      dimension.setY(y);
      dimensionLabel.setY(y);
      screen.addTextLabel(control, id[i]+"aVal", wl_font, "Axis Steps/rev    (aVal):", color(#dfdfdf), dimensionLabel);
      screen.addField(control, id[i]+"aValField", ControlP5.FLOAT, wl_font, color(#FFFFFF), false, dimension).lock();
      y += TEXTBAR_HEIGHT;
      dimension.setY(y);
      dimensionLabel.setY(y);
      screen.addTextLabel(control, id[i]+"sVal", wl_font, "Worm Steps/rev (sVal):", color(#dfdfdf), dimensionLabel);
      screen.addField(control, id[i]+"sValField", ControlP5.FLOAT, wl_font, color(#FFFFFF), false, dimension).lock();
      y += TEXTBAR_HEIGHT;
      dimension.setY(y);
      dimensionLabel.setY(y);
      screen.addTextLabel(control, id[i]+"bVal", wl_font, "Sidereal IRQs/s  (bVal):", color(#dfdfdf), dimensionLabel);
      screen.addField(control, id[i]+"bValField", ControlP5.FLOAT, wl_font, color(#FFFFFF), false, dimension).lock();
      y += TEXTBAR_HEIGHT;
      dimension.setY(y);
      dimensionLabel.setY(y);
      screen.addTextLabel(control, id[i]+"IVal", wl_font, "Sidereal Speed    (IVal):", color(#dfdfdf), dimensionLabel);
      screen.addField(control, id[i]+"IValField", ControlP5.FLOAT, wl_font, color(#FFFFFF), false, dimension).lock();
      y += TEXTBAR_HEIGHT;
      dimension.setY(y);
      dimensionLabel.setY(y);
      screen.addTextLabel(control, id[i]+"Resolution", wl_font, "Resolution   (asec/stp):", color(#dfdfdf), dimensionLabel);
      screen.addField(control, id[i]+"ResolutionField", ControlP5.FLOAT, wl_font, color(#FFFFFF), false, dimension).lock();
      
    }
        
    y = 2*TEXTBAR_HEIGHT+8;
    dimensionLabel = new Dimensions(0*(dimensionMaker.centre()+1)+borderWidth, borderWidth,               0, y,                                            upperLabelWidth, ELEMENT_HEIGHT);
    dimension =      new Dimensions(0*(dimensionMaker.centre()+1)+borderWidth, borderWidth, upperLabelWidth, y, dimensionMaker.width()/2-upperLabelWidth-2-(2*borderWidth), ELEMENT_HEIGHT);
    
    screen.addTextLabel(control, "GuideRateTitle", wl_font, "ST4 Rate (x Sidereal):", color(#ffffff), dimensionLabel);
    screen.addField(control, "raGuideRateField", ControlP5.FLOAT, wl_font, color(#000000), true, dimension);
    
    y += TEXTBAR_HEIGHT;
    dimension.setY(y);
    dimensionLabel.setY(y);
    screen.addTextLabel(control, "advancedHCEnableTitle", wl_font, "Advanced HC Detect:", color(#ffffff), dimensionLabel);
    screen.addButton( control, "raadvancedHCEnableField", "Disabled", wl_font, 1, dimension).setSwitch(true);
    
    y += TEXTBAR_HEIGHT;
    dimension.setY(y);
    dimensionLabel.setY(y);
    Map<String,Integer> entries = new LinkedHashMap <String,Integer>();
    entries.put("A4988/3",DRIVER_A498x);
    entries.put("DRV882x",DRIVER_DRV882x);
    entries.put("DRV8834",DRIVER_DRV8834);
    screen.addTextLabel(control, "driverVersionTitle", wl_font, "Motor Driver IC Type:", color(#ffffff), dimensionLabel);
    screen.addDropdown(control, "radriverVersionField","Driver",wl_font,entries,dimension).setMoveable(false);
        
    y = 2*TEXTBAR_HEIGHT+8;
    dimensionLabel = new Dimensions(1*(dimensionMaker.centre()+1)+borderWidth, borderWidth,               0, y,                                            upperLabelWidth, ELEMENT_HEIGHT);
    dimension =      new Dimensions(1*(dimensionMaker.centre()+1)+borderWidth, borderWidth, upperLabelWidth, y, dimensionMaker.width()/2-upperLabelWidth-2-(2*borderWidth), ELEMENT_HEIGHT);
    
    //screen.addTextLabel(control, "GuideBacklashTitle", wl_font, "Dec. Backlash (Steps):", color(#ffffff), dimensionLabel);
    //screen.addField(control, "raGuideBacklashField", ControlP5.FLOAT, wl_font, color(#000000), true, dimension);
    
    y += TEXTBAR_HEIGHT;
    dimension.setY(y);
    dimensionLabel.setY(y);
    screen.addTextLabel(control, "gearchangeEnableTitle", wl_font, "uStep Gear Changing:", color(#ffffff), dimensionLabel);
    screen.addButton( control, "ragearchangeEnableField", "Disabled", wl_font, 1, dimension).setSwitch(true);
    
    y += TEXTBAR_HEIGHT;
    dimension.setY(y);
    dimensionLabel.setY(y);
    entries = new LinkedHashMap <String,Integer>();
    for (int j = 0; j < 5; j++){
      int mode = (int)pow(2,j);
      entries.put(""+mode+" uStep",mode);
    }
    screen.addTextLabel(control, "microstepEnableTitle", wl_font, "Motor Microstep Level:", color(#ffffff), dimensionLabel);
    screen.addDropdown(control, "ramicrostepEnableField","uSteps",wl_font,entries,dimension).setMoveable(false);
  }

  private void buildProgramConfigScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());

    Dimensions dimension = new Dimensions(0, 0);

    //Add message label
    dimension.setSize(100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addTextLabel(control, "messageBox", wl_font, "", color(#ffffff), dimension);

    //Add progress indicator
    dimension.setSize(dimensionMaker.width()-100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) - TEXTBAR_HEIGHT/2);
    Slider busy = screen.addSlider(control, "busyIndicator", 0, 100, 0, dimension);
    busy.lock();
    busy.getValueLabel().setVisible(false);

    //Add next/back button
    dimension.setSize(50, ELEMENT_HEIGHT);
    dimension.setOrigin(dimensionMaker.width()-100, (dimensionMaker.height()/2) + ELEMENT_HEIGHT);
    screen.addButton( control, "nextButton", "", wl_font, 1, dimension).lock();
  }

  private void buildVersionCheckScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());

    Dimensions dimension = new Dimensions(0, 0);

    //Add message label
    dimension.setSize(100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addTextLabel(control, "messageBox", wl_font, "", color(#ffffff), dimension);

    //Add progress indicator
    dimension.setSize(dimensionMaker.width()-100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) - TEXTBAR_HEIGHT/2);
    Slider busy = screen.addSlider(control, "busyIndicator", 0, 100, 0, dimension);
    busy.lock();
    busy.getValueLabel().setVisible(false);

    //Add next/back button
    dimension.setSize(50, ELEMENT_HEIGHT);
    dimension.setOrigin(dimensionMaker.width()-100, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addButton( control, "nextButton", "", wl_font, 1, dimension).lock();
  }

  private void buildFirmwareFetchScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());

    Dimensions dimension = new Dimensions(0, 0);

    //Add message label
    dimension.setSize(100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addTextLabel(control, "messageBox", wl_font, "", color(#ffffff), dimension);

    //Add progress indicator
    dimension.setSize(dimensionMaker.width()-100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) - TEXTBAR_HEIGHT/2);
    Slider busy = screen.addSlider(control, "busyIndicator", 0, 100, 0, dimension);
    busy.lock();
    busy.getValueLabel().setVisible(false);

    //Add next/back button
    dimension.setSize(50, ELEMENT_HEIGHT);
    dimension.setOrigin(dimensionMaker.width()-100, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addButton( control, "nextButton", "", wl_font, 1, dimension).lock();
  }

  private void buildDownloadFirmwareScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());

    Dimensions dimension = new Dimensions(0, 0);

    //Add message label
    dimension.setSize(100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addTextLabel(control, "messageBox", wl_font, "", color(#ffffff), dimension);

    //Add progress indicator
    dimension.setSize(dimensionMaker.width()-100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) - TEXTBAR_HEIGHT/2);
    Slider busy = screen.addSlider(control, "busyIndicator", 0, 100, 0, dimension);
    busy.lock();
    busy.getValueLabel().setVisible(false);

    //Add next/back button
    dimension.setSize(50, ELEMENT_HEIGHT);
    dimension.setOrigin(dimensionMaker.width()-100, (dimensionMaker.height()/2) + TEXTBAR_HEIGHT/2);
    screen.addButton( control, "nextButton", "", wl_font, 1, dimension).lock();
  }

  private void buildTestFirmwareScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());

    Dimensions dimension = new Dimensions(0, 0);

    //Add message label
    dimension.setSize(100, ELEMENT_HEIGHT);
    dimension.setOrigin(50, (dimensionMaker.height()/2) - TEXTBAR_HEIGHT/2);
    screen.addTextLabel(control, "messageBox", wl_font, "", color(#ffffff), dimension);

    //Add next/back button
    dimension.setSize(50, ELEMENT_HEIGHT);
    dimension.setOrigin(dimensionMaker.width()-100, (dimensionMaker.height()/2) - TEXTBAR_HEIGHT/2);
    screen.addButton( control, "nextButton", "", wl_font, 1, dimension).lock();
  }

  private void buildFinishedScreen(ControlP5 control, GUIScreen screen) {
    println("ID is: "+screen.getGroupID());
    Dimensions dimension = new Dimensions(0, 0);
   
    dimension.setSize(200, 3*ELEMENT_HEIGHT);
    int y = dimensionMaker.height()/2+(ELEMENT_HEIGHT+TEXTBAR_HEIGHT)/2;
    int x = dimensionMaker.width()/2-100;
    
    dimension.setOrigin(x, y-4*TEXTBAR_HEIGHT);
    screen.addButton( control, "finButton", "Finished", wl_font, 1, dimension);
    
    dimension.setOrigin(x,y+TEXTBAR_HEIGHT);
    screen.addButton( control, "returnButton", "Back To Start", wl_font, 1, dimension);
    
    //Add message label
    dimension.setSize(320, ELEMENT_HEIGHT);
    dimension.setOrigin(x-60, y-ELEMENT_HEIGHT);
    screen.addTextLabel(control, "finLabel", wl_font, "", color(#ffffff), dimension);   
   
  }
}