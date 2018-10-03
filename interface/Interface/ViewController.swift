//
//  ViewController.swift
//  Interface
//
//  Created by Joël Gähwiler on 01.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit
import CocoaMQTT

let grid = [
    [ 1,  0,  9,  0, 17,  0],
    [ 0,  5,  0, 13,  0, 21],
    [ 2,  0, 10,  0, 18,  0],
    [ 0,  6, 0,  14,  0, 22],
    [ 3,  0, 11,  0, 19,  0],
    [ 0,  7,  0, 15,  0, 23],
    [ 4,  0, 12,  0, 20,  0],
    [ 0,  8,  0, 16,  0, 24],
]

let rows = grid.count
let columns = grid[0].count

let dotSize = 16

let marginX: Double = 250
let marginY: Double = 200

class ViewController: UIViewController, CocoaMQTTDelegate {
    var container: UIView?
    var circles: [[UIView]]?
    var states: [[Bool]]?
    var client: CocoaMQTT?
    var connected = false
    var timer: Timer?
    
    var fw: Double = 0
    var fh: Double = 0
    var gx: Double = 0
    var gy: Double = 0

    var offColor = UIColor(white: 1, alpha: 0.5)
    var onColor = UIColor(white: 1, alpha: 1)
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // load colors
        let red = UserDefaults.standard.string(forKey: "color-red") ?? "0"
        let green = UserDefaults.standard.string(forKey: "color-green") ?? "0"
        let blue = UserDefaults.standard.string(forKey: "color-blue") ?? "0"
        let cycle = UserDefaults.standard.bool(forKey: "color-cycle")
        
        // save color
        let _red = CGFloat(Float(red) ?? 0)
        let _green = CGFloat(Float(green) ?? 0)
        let _blue = CGFloat(Float(blue) ?? 0)
        onColor = UIColor(red: _red/1023, green: _green/1023, blue: _blue/1023, alpha: 1.0)
        offColor = UIColor(red: _red/1023, green: _green/1023, blue: _blue/1023, alpha: 0.5)
        
        // create container
        container = UIView(frame: view.frame)
        container!.alpha = 0
        view.addSubview(container!)
        
        // set multi touch
        view.isMultipleTouchEnabled = true
        container!.isMultipleTouchEnabled = true
        
        // allocate arrays
        circles = [[UIView]]()
        states = [[Bool]]()
        
        // get frame size
        fw = Double(container!.frame.width)
        fh = Double(container!.frame.height)
        
        // calcuate gaps
        gy = (fh-(2.0*marginY))/Double(rows-1)
        gx = (fw-(2.0*marginX))/Double(columns-1)
        
        // create all circles
        for y in 0..<rows {
            // add rows
            circles!.insert([UIView](), at: y)
            states!.insert([Bool](), at: y)
            
            for x in 0..<columns {
                // calculate position
                let xx = marginX+Double(x)*gx
                let yy = marginY+Double(y)*gy
                
                // create view
                let v = UIView(frame: CGRect(x: xx, y: yy, width: Double(dotSize), height: Double(dotSize)))
                v.backgroundColor = offColor
                v.layer.cornerRadius = CGFloat(dotSize) / 2.0
                
                // check if in grid
                if grid[y][x] == 0 {
                    v.isHidden = true
                }
                
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
        
        // create timer
        if cycle {
            timer = Timer.scheduledTimer(timeInterval: 0.03, target: self, selector: #selector(cycleColors), userInfo: nil, repeats: true)
        }
    }
    
    @objc
    func cycleColors() {
        // get current color
        var hue: CGFloat = 0
        var sat: CGFloat = 0
        var brg: CGFloat = 0
        onColor.getHue(&hue, saturation: &sat, brightness: &brg, alpha: nil)
        
        // set new colors
        onColor = UIColor(hue: hue + 0.001, saturation: sat, brightness: brg, alpha: 1)
        offColor = UIColor(hue: hue + 0.001, saturation: sat, brightness: brg, alpha: 0.5)
        
        // update circles
        for y in 0..<rows {
            for x in 0..<columns {
                if !states![y][x] {
                    circles![y][x].backgroundColor = offColor
                } else {
                    circles![y][x].backgroundColor = onColor
                }
            }
        }
    }
    
    func handleTouches(touches: Set<UITouch>) {
        // handle all touches
        touches.forEach { (touch) in
            handleTouch(touch: touch)
        }
    }
    
    func handleTouch(touch: UITouch) {
        // get location
        let x = Double(touch.location(in: container).x) - Double(marginX)
        let y = Double(touch.location(in: container).y) - Double(marginY)
        
        // calculate row and column number
        let xx = Int(round(x / gx))
        let yy = Int(round(y / gy))
        
        // check bounds
        if xx < 0 || xx >= columns || yy < 0 || yy >= rows {
            return
        }
        
        // get id from grid
        let id = grid[yy][xx]
        if id == 0 {
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
            // get current color
            var red: CGFloat = 0
            var green: CGFloat = 0
            var blue: CGFloat = 0
            onColor.getRed(&red, green: &green, blue: &blue, alpha: nil)
            
            // construct payload
            let payload = String(Int(red * 1023)) + " " + String(Int(green * 1023)) + " " + String(Int(blue * 1023)) + " 0 500"
            
            // perform flash
            client!.publish("lights/" + String(id) + "/flash", withString: payload)
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
