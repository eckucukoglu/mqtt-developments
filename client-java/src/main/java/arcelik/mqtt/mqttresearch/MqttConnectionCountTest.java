package arcelik.mqtt.mqttresearch;

import org.apache.commons.lang.math.RandomUtils;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class MqttConnectionCountTest {

    private String topic        = "connectionCountTestTopic";
    private String content      = "Connection count test message is here!";
    private int qos             = 1;
    private String broker       = "tcp://10.106.231.44:1883";
    private String username     = "admin";
    private String password     = "password";
    private MemoryPersistence persistence = new MemoryPersistence();
    
    public MqttConnectionCountTest() {
        
    }
    
    public void test(int numberOfConnection) {
        
        int count = 0;
        try {
            for (; count < numberOfConnection; count++) {
                int id = RandomUtils.nextInt((int) 1e6);
                MqttClient sampleClient = new MqttClient(broker, String.valueOf(id), persistence);
                
                MqttConnectOptions connOpts = new MqttConnectOptions();
                connOpts.setUserName(username);
                connOpts.setPassword(password.toCharArray());
                connOpts.setCleanSession(true);
                
                System.out.println("Client " + id + " is connecting to broker: " + broker);
                
                sampleClient.connect(connOpts);
                
                System.out.println("Connected");
                
//                System.out.println("Publishing message: " + content);
//                MqttMessage message = new MqttMessage(content.getBytes());
//                message.setQos(qos);
//                
//                sampleClient.publish(topic, message);
//                System.out.println("Message published");
//                
                sampleClient.disconnect();
                System.out.println("Disconnected");
            }
            
            Thread.sleep(20000);
            
        } catch(MqttException me) {
            System.out.println("reason " + me.getReasonCode());
            System.out.println("msg " + me.getMessage());
            System.out.println("loc " + me.getLocalizedMessage());
            System.out.println("cause " + me.getCause());
            System.out.println("excep " + me);
            me.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            System.out.println("Count: " + count);
        }
    }
}
