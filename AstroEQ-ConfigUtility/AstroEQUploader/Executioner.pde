import processing.serial.*;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.*;
import java.io.*;
import java.net.URL;

class TaskStatus {

  private boolean _running=false;
  private boolean _complete=false;
  private int _error = 0;
  
  public synchronized void setStatus(boolean running, boolean complete, int error) {
    _running = running;
    _complete = complete;
    _error = error;
  }
  
  public synchronized void setRunning(boolean running) {
    _running = running;
  }

  public synchronized void setComplete(boolean complete) {
    _complete = complete;
  }

  public synchronized void setError(int error) {
    _error = error;
  }
  
  public synchronized boolean isRunning() {
    return _running;
  }

  public synchronized boolean isComplete() {
    return _complete;
  }

  public synchronized boolean isError() {
    return (_error != 0);
  }
  public synchronized int errorCode() {
    return _error;
  }
}

public abstract class Executioner implements Runnable {

  protected TaskStatus execStatus;
  protected PApplet p;
  protected String[] execArgs;
  protected List<String> buffer = new ArrayList<String>();

  public Executioner(PApplet _p) {
    p = _p;
    execStatus = new TaskStatus();
  }

  public synchronized void setArgs( String[] args ) {
    execArgs = args;
  }
  
  public synchronized TaskStatus status() {
    return execStatus;
  }
  
  public synchronized void resetStatus(){
    execStatus.setStatus(false,false,0);
  }

  public synchronized String[] getOutput() {
    String[] these = buffer.toArray(new String[buffer.size()]);
    buffer.clear();
    return(these);
  }
  
  abstract void run();
}

public class NullInterface extends Executioner {
  
  NullInterface (PApplet _p){
    super(_p);
  }
  
  public void run(){
    execStatus.setStatus(false,true,1);
    return;
  }
}

public final ResponseLookup responseLookup = new ResponseLookup();
public class ResponseLookup {
  public final Map<String, Object[]> resp = responseLookupMaker();
  public Object[] get (String str) {
    return resp.get(str);
  }
  public String find(String obj){
    for (Map.Entry entry : resp.entrySet()) {
        Object[] value = (Object[])entry.getValue();
        if (obj.equals((String)value[0])) {
            return (String)entry.getKey();
        }
    }
    return null;
  }
  private Map<String, Object[]> responseLookupMaker() {
      Map<String, Object[]> mapGen = new HashMap<String, Object[]>();
      
      Object[] temp = new Object[2];
      temp[0]="aVal";
      temp[1]=SyntaString.argIsLong;
      mapGen.put("a",temp);
      
      temp = new Object[2];
      temp[0]="bVal";
      temp[1]=SyntaString.argIsLong;
      mapGen.put("b",temp);
      
      temp = new Object[2];
      temp[0]="sVal";
      temp[1]=SyntaString.argIsLong;
      mapGen.put("s",temp);
      
      temp = new Object[2];
      temp[0]="IVal";
      temp[1]=SyntaString.argIsLong;
      mapGen.put("n",temp);
      
      temp = new Object[2];
      temp[0]="axisReverse";
      temp[1]=SyntaString.argIsByte;
      mapGen.put("c",temp);
      
      temp = new Object[2];
      temp[0]="driverVersion";
      temp[1]=SyntaString.argIsByte;
      mapGen.put("d1",temp);
      
      temp = new Object[2];
      temp[0]="microstepEnable";
      temp[1]=SyntaString.argIsByte;
      mapGen.put("d2",temp);
      
      temp = new Object[2];
      temp[0]="advancedHCEnable";
      temp[1]=SyntaString.argIsByte;
      mapGen.put("q1",temp);
      
      temp = new Object[2];
      temp[0]="gearchangeEnable";
      temp[1]=SyntaString.argIsByte;
      mapGen.put("q2",temp);
      
      temp = new Object[2];
      temp[0]="GuideRate";
      temp[1]=SyntaString.argIsLong;
      mapGen.put("r1",temp);
      
      temp = new Object[2];
      temp[0]="GuideBacklash";
      temp[1]=SyntaString.argIsLong;
      mapGen.put("r2",temp);
      
      temp = new Object[2];
      temp[0]="Goto";
      temp[1]=SyntaString.argIsByte;
      mapGen.put("z",temp);
      
      temp = new Object[2];
      temp[0]="Angle";
      temp[1]=SyntaString.argIsLong;
      mapGen.put("V",temp);
      
      temp = new Object[2];
      temp[0]="UGear";
      temp[1]=SyntaString.argIsLong;
      mapGen.put("U",temp);
      
      temp = new Object[2];
      temp[0]="WGear";
      temp[1]=SyntaString.argIsLong;
      mapGen.put("W",temp);
      
      return Collections.unmodifiableMap(mapGen);
  }
}

public class EEPROMInterface extends Executioner {
  
  
  protected ConcurrentLinkedQueue<Character> inQueue;
  protected ConcurrentLinkedQueue<Character> outQueue;
  
  EEPROMInterface (PApplet _p, ConcurrentLinkedQueue<Character> _inQueue, ConcurrentLinkedQueue<Character> _outQueue){
    super(_p);
    outQueue = _outQueue;
    inQueue = _inQueue;
  }
  
  private void write(String out) {
    for (char ch: out.toCharArray()) {
      outQueue.offer(new Character(ch)); //Output each character in the string to our output queue
    }
  }
  
  public void run() {
    int exitCode = 0;
    execStatus.setStatus(true,false,exitCode);
    buffer.clear();
    
    println(execArgs.length);
    List<String> args = new ArrayList<String>();
    Collections.addAll(args, execArgs); 
    
    String mode = args.get(0);
    args.remove(0);
    
    println(args.size());
    
    if ((inQueue == null) || (outQueue == null)) {
      println("Queues not initialised.");
      execStatus.setStatus(false,true,4);
      return;
    }
    
    try {
      Thread.sleep(3500); //Ensure Arduino has finished reset.
    } catch (Exception e){
      
    }
    write(":O1"+mode+"\r");
    buffer.add(":O1"+mode);
    println(":O1"+mode);
    String readback = getResponse(10000,'\r');
    println(readback);
    if((readback==null) || !readback.equals("=\r")){ //try to find a response within 10 seconds
      exitCode = 3; //could not set mode, fatal error
      println("Failed to recieve response.");
    }
    
    while ((args.size() > 0) && (exitCode == 0)) {
      String arg = args.get(0);
      args.remove(0);
      if(arg.charAt(0) != ':') {
        continue; //skip this as its and invalid command
      }
      println(arg);
      buffer.add(arg);
      write(arg+"\r");
      
      readback = getResponse(20000,'\r');
      if(readback==null){
        exitCode = 2; //no response, communication lost.
        break;
      } else if (mode.charAt(0)=='1'){
        String fetch = readback;
        if (fetch.equals("!\r")) {
          exitCode = 1; //empty response. Not a fatal error as there was a response, but it means blank eeprom.
          break;
        }
        println(fetch);
        Object[] responseDecoder = responseLookup.get(arg.substring(1,2));
        if (responseDecoder != null){
          boolean decodeLength = (Boolean)responseDecoder[1];
          String decodedValue = SyntaString.syntaEncoder(fetch.substring(1,fetch.length()-1),decodeLength,SyntaString.decode);
          String decodedString = ((arg.charAt(2)=='1')?"ra":"dc")+(String)responseDecoder[0]+decodedValue;
          buffer.add(decodedString);
          println(decodedString);
        } else {
          responseDecoder = responseLookup.get(arg.substring(1,3));
          if (responseDecoder != null){
            boolean decodeLength = (Boolean)responseDecoder[1];
            String decodedValue = SyntaString.syntaEncoder(fetch.substring(1,fetch.length()-1),decodeLength,SyntaString.decode);
            String decodedString = "ra"+(String)responseDecoder[0]+decodedValue;
            buffer.add(decodedString);
            println(decodedString);
          }
        }
      } else if (!readback.equals("=\r")) {
        exitCode = 2; //command failed response, communication lost.
        break;
      }
    }
        
    execStatus.setStatus(false,true,exitCode);
    return;
  
  }
  
  private String getResponse(int timeout, int marker){
    String readback = "";
    long startTimeMillis = System.currentTimeMillis();
    try {
      println("Awaiting Response...");
      while (true) {
        Character ch = (Character)inQueue.poll();
        if(ch != null){
          int i = ch.hashCode();
          readback = readback+(char)i;
          println(i);
          if(i == marker) {
            return readback;
          }
        }
        if ((System.currentTimeMillis() - startTimeMillis) > timeout) {
          return null; //timed out, so return null
        }
      }
    } catch (Exception e) {
      e.printStackTrace();
      return null; //error!
    }
  }
}

class NewFirmwareFetcher extends Executioner {
  
  NewFirmwareFetcher (PApplet _p){
    super(_p);
  }
  
  public void run() {
    int exitCode = 0;
    execStatus.setStatus(true,false,exitCode);
    buffer.clear();
    buffer.add("Fetching New Files..."); //print to buffer
    println("Fetching New Files...");
    List<String> fileList = new ArrayList<String>();
    BufferedInputStream remoteFile = null;
    FileOutputStream newFile = null;
    try {
      String marker = "FOUND:\t";
      for (String arg : execArgs) {
        if (arg.startsWith(marker)){
          //this line is a file to fetch
          String[] parts = arg.split("\t"); //this will split the argument up. [0] will be the marker [1] will be the filename [2] doesn't matter
          fileList.add(parts[1]); //add the filename onto the list to add.
          println(parts[1]);
        }
      }
      if (fileList.size() == 0){
        buffer.add("No files to fetch!");
        println("No files to fetch!");
        exitCode = 1; //no files to fetch.
      } else {
        fileList.add(versionFilename); //as we are fetching files, we also need to get the new file list.
        String urlPrefix = "https://github.com/TCWORLD/AstroEQ/raw/master/AstroEQ-ConfigUtility/hex/";
        String[] filesToFetch = fileList.toArray(new String[fileList.size()]);
        for(String filename : filesToFetch){
          String extension = (filename.equals(versionFilename)?"":".hex");
          String outputExtension = (filename.equals(versionFilename)?"":".hex");
          println("Fetching: "+filename+extension);
          buffer.add("Fetching: "+filename+extension);
          remoteFile = new BufferedInputStream(new URL(""+urlPrefix+filename+extension).openStream());
          newFile = new FileOutputStream("" + hexPath + java.io.File.separator + filename+outputExtension);
          byte data[] = new byte[1024];
          int count;
          while ((count = remoteFile.read(data, 0, 1024)) != -1)
          {
              newFile.write(data, 0, count);
          }
          remoteFile.close();
          newFile.close();
        }
        buffer.add("All files fetched!");
        println("All files fetched!");
      }      
    } catch (Exception err) { 
      err.printStackTrace();
      exitCode = 2;
      buffer.add("Error fetching files!!!"); //print to buffer
      println("Error fetching files!!!");
    }
    execStatus.setStatus(false,true,exitCode);
    println("running = "+execStatus.isRunning());
    println("complete = "+execStatus.isComplete());
    println("error = "+execStatus.isError());
  }
}

class FileUpdateCheck extends Executioner {
  
  FileUpdateCheck (PApplet _p){
    super(_p);
  }
  
  public void run() {
    int exitCode = 0;
    execStatus.setStatus(true,false,exitCode);
    buffer.clear();
    buffer.add("Finding New Files..."); //print to buffer
    println("Finding New Files...");
    BufferedReader reader = null;
    try {
      // Create a URL for the desired page
      Map<String, String> files = new HashMap<String, String>();
      reader = new BufferedReader(createReader("" + hexPath + java.io.File.separator + versionFilename));
      String str;
      while ((str = reader.readLine()) != null) {
        if(str.contains("\t")){
          println(str);
          String[] parts = str.split("\t");
          files.put(parts[0],parts[1]); //filename then version.
        }
      }
      reader.close();
      println("Local Versions Read");
      println();
      URL url = new URL("https://github.com/TCWORLD/AstroEQ/raw/master/AstroEQ-ConfigUtility/hex/"+versionFilename);
      //URL url = new URL("http://www.astroeq.co.uk/"+versionFilename);
      reader = new BufferedReader(new InputStreamReader(url.openStream()));
      while ((str = reader.readLine()) != null) {
        if(str.contains("\t")){
          String[] parts = str.split("\t");
          if (files.containsKey(parts[0])){
            println(str);
            String version = files.get(parts[0]);
            Double currentVersion = Double.parseDouble(version);
            Double remoteVersion;
            remoteVersion = Double.parseDouble(parts[1]);
            if(remoteVersion > currentVersion){ //check if version on server is newer.
              buffer.add("FOUND:\t"+parts[0]+"\t\tV"+version+"--->V"+parts[1]); //add file to the buffer.
              println(parts[0]);
              if (parts[0].equals("AstroEQUploaderUtility")){
                exitCode = 3;
                buffer.add("INFO:\tNew Version of AstroEQUploader Utility Available!"); //print to buffer
                buffer.add("Please Download From: http://www.astroeq.co.uk/downloads.php");
                println("INFO:\tNew Version of AstroEQUploader Utility Available!");
                break;
              }
              exitCode = 1; //There are new files.
            }
          }
        }
      }
      reader.close();
      buffer.add("Search Complete."); //print to buffer
      println("Search Complete.");
    } catch (Exception e) {
      buffer.add("Error finding files!!!"); //print to buffer
      println("Error finding files!!!");
      exitCode = 2; //An error occured, most likely due to no internet connection.
      e.printStackTrace();
    }
    execStatus.setStatus(false,true,exitCode);
    println("running = "+execStatus.isRunning());
    println("complete = "+execStatus.isComplete());
    println("error = "+execStatus.isError());
  }
}

class ExecutableInterface extends Executioner {
  
  ExecutableInterface (PApplet _p){
    super(_p);
  }
  
  public void run() {
    int exitCode = 0;
    
    execStatus.setStatus(true,false,exitCode);
    buffer.clear();
  
    try {
      // create a single-string commandline
      if(execArgs == null) {
        return; //don't bother running as there is no command.
      }
//      String commandLine = execArgs[0];
//
//      for (int i = 1; i < execArgs.length ; i++) {
//        commandLine = commandLine + " " + execArgs[i];
//      }

      String line;

      Process p = Runtime.getRuntime().exec(execArgs);

      BufferedReader input = new BufferedReader(new InputStreamReader(p.getErrorStream()));

      while ( (line = input.readLine ()) != null) {
        // look for programmer timeout
        if ( line.length() > 37 && line.substring(0, 38).equals("avrdude: stk500_getsync(): not in sync") ) {
          println("Timeout Error");
          p.destroy();
        } 
        else if ( line.length() > 42 && line.substring(0, 43).equals("avrdude: stk500v2_ReceiveMessage(): timeout") ) {
          println("Timeout Error");
          p.destroy();
        }
        // hand buffered line over
        buffer.add(line);
        println(line);
      }
      exitCode = p.waitFor();
      println("Exit code : " + exitCode);
    } 
    catch (Exception err) { 
      err.printStackTrace();
    }
    execStatus.setStatus(false,true,exitCode);
    println("running = "+execStatus.isRunning());
    println("complete = "+execStatus.isComplete());
    println("error = "+execStatus.isError());
  }
}