import java.io.IOException;

import java.lang.System;

import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

public class TestPlugin extends Plugin
{

   public TestPlugin()
   {


      byte[] initSize = new byte[] {(byte)0xfc, (byte)0xff, (byte)0xff, (byte)0xff};
      byte[] initPayload = ByteBuffer.allocate(4).putInt(0).array();

      VdmsTransaction initSequence = new VdmsTransaction(initSize, initPayload);

         String sourceNodes = System.getenv("SOURCES");
         String sourceNodesArray[] = sourceNodes.split(",");
         int sourceNodeCount = sourceNodesArray.length;
         String destinationNodes = System.getenv("DESTINATIONS");
         String destinationNodesArray[] = destinationNodes.split(",");
         int destinationNodeCount = destinationNodesArray.length;

         for(int i = 0; i < sourceNodeCount; i++)
      {
            String connectionInfo[] = sourceNodesArray[i].split(":");
            int connectionPort = Integer.parseInt(connectionInfo[1]);
            System.out.println(connectionInfo[0] + " " + Integer.toString(connectionPort));
            try 
            {
               Socket thisSocket = new Socket(connectionInfo[0], connectionPort);
               PublisherServiceThread thisServerServivceThread = new PublisherServiceThread(this, thisSocket, threadId, initSequence);
               AddNewProducer(thisServerServivceThread);
            }
            catch (UnknownHostException e)
         {
            e.printStackTrace();
         }
            catch (IOException e)
         {
            e.printStackTrace();
         }
      }
         
         for(int i = 0; i < destinationNodeCount; i++)
      {
            String connectionInfo[] = destinationNodesArray[i].split(":");
            int connectionPort = Integer.parseInt(connectionInfo[1]);
            System.out.println(connectionInfo[0] + " " + Integer.toString(connectionPort));
            try 
         {
               Socket thisSocket = new Socket(connectionInfo[0], connectionPort);
               SubscriberServiceThread thisServerServivceThread = new SubscriberServiceThread(this, thisSocket, threadId, null, null);
               AddNewConsumer(thisServerServivceThread);
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
         
      //Start all of the threads. We start the destination nodes first so that no data from the sources is acceptd
      for(int i = 0; i < producerList.size(); i++)
      {
         producerList.get(i).start();
      }
      for(int i = 0; i < consumerList.size(); i++)
      {
         consumerList.get(i).start();
      }
    }


   public static void main (String[] args)
   {
      new TestPlugin();
   }

}

