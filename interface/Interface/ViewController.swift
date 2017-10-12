//
//  ViewController.swift
//  Interface
//
//  Created by Joël Gähwiler on 01.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit
import CocoaMQTT

let lightsPerRow = 8
let lightsPerColumn = 6
let lightDotSize = 12
let padding: Double = 150

class ViewController: UIViewController, CocoaMQTTDelegate {
    var container: UIView?
    var circles: [[UIView]]?
    var states: [[Bool]]?
    var client: CocoaMQTT?
    var connected = false
    
    var fw: Double = 0
    var fh: Double = 0
    var gx: Double = 0
    var gy: Double = 0
    
    var red: String = "0"
    var green: String = "0"
    var blue: String = "0"
    var white: String = "0"
    
    var offColor = UIColor(white: 0.1, alpha: 1)
    var onColor = UIColor(white: 1, alpha: 1)
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // load colors
        red = UserDefaults.standard.string(forKey: "color-red") ?? "0"
        green = UserDefaults.standard.string(forKey: "color-green") ?? "0"
        blue = UserDefaults.standard.string(forKey: "color-blue") ?? "0"
        white = UserDefaults.standard.string(forKey: "color-white") ?? "0"
        
        // save color
        let _red = CGFloat(Float(red) ?? 0)
        let _green = CGFloat(Float(green) ?? 0)
        let _blue = CGFloat(Float(blue) ?? 0)
        onColor = UIColor(red: _red/1023, green: _green/1023, blue: _blue/1023, alpha: 1.0)
        offColor = UIColor(red: _red/1023, green: _green/1023, blue: _blue/1023, alpha: 0.25)
        
        // create container
        container = UIView(frame: view.frame)
        container!.alpha = 0
        view.addSubview(container!)
        
        // allocate arrays
        circles = [[UIView]]()
        states = [[Bool]]()
        
        // get frame size
        fw = Double(container!.frame.width)
        fh = Double(container!.frame.height)
        
        // calcuate gaps
        gx = (fw-(2.0*padding))/Double(lightsPerRow-1)
        gy = (fh-(2.0*padding))/Double(lightsPerColumn-1)
        
        // create all circles
        for y in 0..<lightsPerColumn {
            // add rows
            circles!.insert([UIView](), at: y)
            states!.insert([Bool](), at: y)
            
            for x in 0..<lightsPerRow {
                // calculate position
                let xx = padding+Double(x)*gx
                let yy = padding+Double(y)*gy
                
                // create view
                let v = UIView(frame: CGRect(x: xx, y: yy, width: Double(lightDotSize), height: Double(lightDotSize)))
                v.backgroundColor = offColor
                v.layer.cornerRadius = CGFloat(lightDotSize) / 2.0
                
                // add to view
                container!.addSubview(v)
                
                // add to arrays
                circles![y].insert(v, at: x)
                states![y].insert(false, at: x)
            }
        }
        
        // get settings
        let host = UserDefaults.standard.string(forKey: "host") ?? ""
        let clientID = UserDefaults.standard.string(forKey: "client-id") ?? ""
        let username = UserDefaults.standard.string(forKey: "username")
        let password = UserDefaults.standard.string(forKey: "password")
        
        // create client
        client = CocoaMQTT(clientID: clientID, host: host, port: 1883)
        client!.username = username
        client!.password = password
        client!.delegate = self
        client!.autoReconnect = true
        client!.autoReconnectTimeInterval = 1
        
        // connect to broker
        client!.connect()
    }
    
    func handleTouches(touches: Set<UITouch>) {
        // handle all touches
        touches.forEach { (touch) in
            handleTouch(touch: touch)
        }
    }
    
    func handleTouch(touch: UITouch) {
        // get location
        let x = Double(touch.location(in: container).x) - Double(padding)
        let y = Double(touch.location(in: container).y) - Double(padding)
        
        // calculate row and column number
        let xx = Int(round(x / gx))
        let yy = Int(round(y / gy))
        
        // check bounds
        if xx < 0 || xx >= lightsPerRow || yy < 0 || yy >= lightsPerColumn {
            return
        }
        
        // check state
        if states![yy][xx] {
            return
        }
        
        // get view from array
        let v = circles![yy][xx]
        
        // send message
        if connected {
            // client!.publish("lights/" + String(yy*lightsPerRow+xx+1) + "/flash", withString: "500")
            let payload = red + " " + green + " " + blue + " " + white + " 500"
            client!.publish("lights/" + String(yy*lightsPerRow+xx+1) + "/flash-color", withString: payload)
        }
        
        // set state
        states![yy][xx] = true
        
        // animate circle
        UIView.animate(withDuration: 0.25, delay: 0.0, animations: {
            // increase intensity
            v.backgroundColor = self.onColor
        }, completion: { finished in
            UIView.animate(withDuration: 0.25, delay: 0.0, animations: {
                // decrease intensity
                v.backgroundColor = self.offColor
            }, completion: { finished in
                // reset state
                self.states![yy][xx] = false
            })
        })
    }
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouches(touches: touches)
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouches(touches: touches)
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouches(touches: touches)
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didConnectAck ack: CocoaMQTTConnAck) {
        // return immediately if connection has been rejected
        if ack != .accept {
            return
        }
        
        // set flag
        connected = true
        
        // animate container
        UIView.animate(withDuration: 0.25, delay: 0.0, animations: {
            // set transparency
            self.container!.alpha = 1
        })
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishMessage message: CocoaMQTTMessage, id: UInt16) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishAck id: UInt16) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didReceiveMessage message: CocoaMQTTMessage, id: UInt16) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didSubscribeTopic topic: String) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didUnsubscribeTopic topic: String) {}
    
    func mqttDidPing(_ mqtt: CocoaMQTT) {}
    
    func mqttDidReceivePong(_ mqtt: CocoaMQTT) {}
    
    func mqttDidDisconnect(_ mqtt: CocoaMQTT, withError err: Error?) {
        // set flag
        connected = false
        
        // animate container
        UIView.animate(withDuration: 0.25, delay: 0.0, animations: {
            // set transparency
            self.container!.alpha = 0
        })
    }
}
