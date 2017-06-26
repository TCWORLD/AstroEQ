/*
 * From https://processing.org/discourse/beta/num_1274718629.html
 */
 
public class ClipHelper {
  Clipboard clipboard;
  
  ClipHelper() {
    getClipboard();  
  }
  
  void getClipboard () {
    // this is our simple thread that grabs the clipboard
    Thread clipThread = new Thread() {
  public void run() {
    clipboard = Toolkit.getDefaultToolkit().getSystemClipboard();
  }
    };
  
    // start the thread as a daemon thread and wait for it to die
    if (clipboard == null) {
  try {
    clipThread.setDaemon(true);
    clipThread.start();
    clipThread.join();
  }  
  catch (Exception e) {}
    }
  }
  
  void copyString (String data) {
    copyTransferableObject(new StringSelection(data));
  }
  
  void copyTransferableObject (Transferable contents) {
    getClipboard();
    clipboard.setContents(contents, null);
  }
  
  String pasteString () {
    String data = null;
    try {
  data = (String)pasteObject(DataFlavor.stringFlavor);
    }  
    catch (Exception e) {
  System.err.println("Error getting String from clipboard: " + e);
    }
    return data;
  }
  
  Object pasteObject (DataFlavor flavor)  
  throws UnsupportedFlavorException, IOException
  {
    Object obj = null;
    getClipboard();
    
    Transferable content = clipboard.getContents(null);
    if (content != null)
    obj = content.getTransferData(flavor);
    
    return obj;
  }
}
