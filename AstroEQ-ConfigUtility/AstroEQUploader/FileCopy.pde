import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import javax.swing.filechooser.FileFilter;

public class FileCopy {
  public void copyFile(String sourceFile, String destinationFile) {                
    try {                        
      FileInputStream fileInputStream = new FileInputStream(sourceFile);                        
      FileOutputStream fileOutputStream = new FileOutputStream(destinationFile);                        
      int bufferSize;                        
      byte[] buffer = new byte[512];                        
      while ( (bufferSize = fileInputStream.read (buffer)) > 0) {                                
        fileOutputStream.write(buffer, 0, bufferSize);
      }                        
      fileInputStream.close();                        
      fileOutputStream.close();
    } 
    catch (Exception e) {                        
      e.printStackTrace();
    }
  }
}

public class ExtensionFilter extends FileFilter {
  private String extensions[];

  private String description;

  public ExtensionFilter(String description, String extension) {
    this(description, new String[] { extension });
  }

  public ExtensionFilter(String description, String extensions[]) {
    this.description = description;
    this.extensions = (String[]) extensions.clone();
  }

  public boolean accept(File file) {
    if (file.isDirectory()) {
      return true;
    }
    int count = extensions.length;
    String path = file.getAbsolutePath();
    for (int i = 0; i < count; i++) {
      String ext = extensions[i];
      if (path.endsWith(ext)
          && (path.charAt(path.length() - ext.length()) == '.')) {
        return true;
      }
    }
    return false;
  }

  public String getDescription() {
    return (description == null ? extensions[0] : description);
  }
}
