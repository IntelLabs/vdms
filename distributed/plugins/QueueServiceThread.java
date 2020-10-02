

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;

   class QueueServiceThread extends Thread 
   { 
      BlockingQueue<VdmsTransaction> queue;
      RepeaterPlugin manager;
      
      public QueueServiceThread(BlockingQueue<VdmsTransaction> nQueue, RepeaterPlugin nManager)
      {
         queue = nQueue;
         manager = nManager;
      }

      public void run()
      {
         VdmsTransaction message;
         try
         {
            while(true)
            {
               message = queue.take();
               /*for(int i = 0; i < manager.GetThreadTypeArray().size(); i++)
               {
                  if(manager.GetThreadTypeArray().get(i) == matchType)
                  {
                     manager.GetThreadArray().get(i).Publish(message);
                  }
               }
               */
            }

         }
         catch(InterruptedException e)
         {
            this.interrupt();
            
         }


      }

   }
