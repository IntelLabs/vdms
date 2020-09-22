import java.io.BufferedReader;
import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;


import java.lang.System;

import java.nio.ByteBuffer;

import java.net.ServerSocket;
import java.net.Socket;
import java.net.ExtendedSocketOptions;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.List;
import java.util.Arrays;
import java.util.ArrayList;


public class PluginSample {
   ServerSocket myServerSocket;
   boolean ServerOn = true;
        
    public PluginSample(WorkerConnectionList workerConnections) { 
      try {
	  myServerSocket = new ServerSocket(Integer.parseInt(System.getenv("NETWORK_PORT")));
      } catch(IOException ioe) { 
         System.out.println("Could not create server socket on port 8888. Quitting.");
         System.exit(-1);
      } 
		
      Calendar now = Calendar.getInstance();
      SimpleDateFormat formatter = new SimpleDateFormat(
         "E yyyy.MM.dd 'at' hh:mm:ss a zzz");
      System.out.println("It is now : " + formatter.format(now.getTime()));
      
      while(ServerOn) { 
         try { 
            Socket clientSocket = myServerSocket.accept();
            ClientServiceThread cliThread = new ClientServiceThread(clientSocket, workerConnections);
            cliThread.start(); 
         } catch(IOException ioe) { 
            System.out.println("Exception found on accept. Ignoring. Stack Trace :"); 
            ioe.printStackTrace(); 
         }  
      } 
      try { 
         myServerSocket.close(); 
         System.out.println("Server Stopped"); 
      } catch(Exception ioe) { 
         System.out.println("Error Found stopping server socket"); 
         System.exit(-1); 
      } 
   }

    
   public static void main (String[] args) {

       String workerNodes = System.getenv("WORKERS");
       String workerNodesArray[] = workerNodes.split(",");
       int workerNodeCount = workerNodesArray.length;
       WorkerConnectionList workerConnections = new WorkerConnectionList();
       
       for(int i = 0; i < workerNodeCount; i++)
	   {
	       String connectionInfo[] = workerNodesArray[i].split(":");
	       int connectionPort = Integer.parseInt(connectionInfo[1]);
	       System.out.println(connectionInfo[0] + " " + Integer.toString(connectionPort));
	       try (Socket thisSocket = new Socket(connectionInfo[0], connectionPort))
		   {
		       
		       workerConnections.AddWorker(new WorkerConnection(thisSocket));
		       System.out.println("Socket is " + Boolean.toString(thisSocket.isConnected()));
		   }
	       catch (UnknownHostException ex)
		   {
		       System.out.println("Server not found: " + ex.getMessage());
		   }
	       catch (IOException ex)
		   {
		       System.out.println("I/O error: " + ex.getMessage());
		   }
	   }
       
       System.out.println(workerNodes);
       
      new PluginSample(workerConnections);        
   }




    

    
   class ClientServiceThread extends Thread { 
      Socket myClientSocket;
      boolean m_bRunThread = true;
       WorkerConnectionList workerConnections;
      public ClientServiceThread() { 
         super(); 
      } 
		
       ClientServiceThread(Socket s, WorkerConnectionList nWorkerConnections) { 
         myClientSocket = s;
	 workerConnections = nWorkerConnections;
      } 
		
      public void run() { 
         DataInputStream in = null; 
         DataOutputStream out = null;

	 byte[] readSizeArray = new byte[4];
	 
	 
         System.out.println(
            "Accepted Client Address - " + myClientSocket.getInetAddress().getHostName());
	 System.out.println(System.getenv("PATH"));
         try { 
	     in = new DataInputStream(myClientSocket.getInputStream());
	     out = new DataOutputStream(myClientSocket.getOutputStream());
            
            while(m_bRunThread) { 
		int clientCommand = in.read(readSizeArray, 0, 4);
		int int0 = readSizeArray[0];
		int int1 = readSizeArray[1];
		int int2 = readSizeArray[2];
		int int3 = readSizeArray[3];
		int readSize = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
		//now i can read the rest of the data
		

		byte[] buffer = new byte[readSize];
		clientCommand = in.read(buffer, 0, readSize);
		System.out.println("readsizearray - " + Arrays.toString(readSizeArray));
		System.out.println("buffer - " + Arrays.toString(buffer));
						   
		int tmpVal = workerConnections.GetSize();
		VdmsTransaction outMessage = new VdmsTransaction(readSizeArray, buffer);
		VdmsTransaction inMessage = workerConnections.Publish(outMessage);

		out.write(inMessage.GetSize());
		out.write(inMessage.GetBuffer());

		workerConnections.ClearResults();
		buffer = null;
		//make the value null after it is used
		System.gc();

		
		//System.out.println("Client Says :" + Integer.toString(tmpInt));
		if(!ServerOn) { 
		System.out.print("Server has already stopped"); 
		//out.println("Server has already stopped"); 
		    out.flush(); 
		    m_bRunThread = false;
		} 
		if(clientCommand == -1) {
		    m_bRunThread = false;
                  System.out.print("Stopping client thread for client : ");
               } else if(clientCommand == -1) {
                  m_bRunThread = false;
                  System.out.print("Stopping client thread for client : ");
                  ServerOn = false;
               } else {
		    //out.println("Server Says : " + clientCommand);
                  out.flush(); 
               } 
            } 
         } catch(Exception e) { 
            e.printStackTrace(); 
         } 
         finally { 
            try { 
               in.close(); 
               out.close(); 
               myClientSocket.close(); 
               System.out.println("...Stopped"); 
            } catch(IOException ioe) { 
               ioe.printStackTrace(); 
            } 
         } 
      } 
   }



}

