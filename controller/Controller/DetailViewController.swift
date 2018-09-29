//
//  DetailViewController.swift
//  Controller
//
//  Created by Joël Gähwiler on 08.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit

let lightWidth: Double = 10
let lightLength: Double = 200
let objectWidth: Double = 100
let floorWidth: Double = 300
let floorHeight: Double = 3
let bottomPadding: Double = 50

class DetailViewController: UIViewController {
    var mainVC: MainViewController?
    
    var ropeView: UIView?
    var lightView: UIView?
    var floorView: UIView?
    var objectView: UIView?
    var moveView: UIView?
    
    @IBOutlet var idLabel: UILabel?
    @IBOutlet var positionLabel: UILabel?
    @IBOutlet var distanceLabel: UILabel?
    @IBOutlet var motionLabel: UILabel?
    @IBOutlet var moveLabel: UILabel?
    @IBOutlet var stateLabel: UILabel?
    
    var id = 0
    
    var position: Double {
        get { return Double(positionLabel?.text ?? "0") ?? 0 }
        set {
            positionLabel?.text = String(format: "%.1f", newValue)
            recalculate()
        }
    }
    
    var distance: Double {
        get { return Double(distanceLabel?.text ?? "0") ?? 0 }
        set {
            distanceLabel?.text = String(format: "%.1f", newValue)
            recalculate()
        }
    }
    
    var motion: Bool {
        get { return motionLabel?.text == "YES" }
        set {
            motionLabel?.text = newValue ? "YES" : "NO"
            recalculate()
        }
    }
    
    var state: String {
        get { return stateLabel?.text ?? "" }
        set { stateLabel?.text = newValue }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // write id
        idLabel!.text = String(format: "%02d", id)
        
        // create rope view
        ropeView = UIView()
        ropeView!.layer.borderWidth = 1
        ropeView!.layer.borderColor = UIColor.white.cgColor
        view.addSubview(ropeView!)
        
        // create light view
        lightView = UIView()
        lightView!.backgroundColor = UIColor.white
        view.addSubview(lightView!)
        
        // create floor view
        floorView = UIView()
        floorView!.layer.borderWidth = 3
        floorView!.layer.borderColor = UIColor.white.cgColor
        view.addSubview(floorView!)
        
        // create object view
        objectView = UIView()
        objectView!.backgroundColor = UIColor.black
        view.addSubview(objectView!)
        
        // create move view
        moveView = UIView()
        moveView!.alpha = 0
        moveView!.layer.borderWidth = 1
        moveView!.layer.borderColor = UIColor.white.cgColor
        view.addSubview(moveView!)
        
        // recalculate frames
        recalculate()
    }
    
    func recalculate() {
        // calculate dimensions
        let fw = Double(view.frame.width)
        let fh = Double(view.frame.height)
        let objectHeight = position-distance
        let ropeLength = fh-bottomPadding-objectHeight-distance-lightLength
        
        // set frames
        ropeView!.frame = CGRect(x: fw/2, y: 0, width: 1, height: ropeLength)
        lightView!.frame = CGRect(x: fw/2 - lightWidth/2, y: ropeLength, width: lightWidth, height: lightLength)
        floorView!.frame = CGRect(x: fw/2 - floorWidth/2, y: fh-bottomPadding, width: floorWidth, height: floorHeight)
        objectView!.frame = CGRect(x: fw/2 - objectWidth/2, y: fh-bottomPadding-objectHeight, width: objectWidth, height: objectHeight)
    }
    
    @IBAction func stop() {
        send(topic: "stop", payload: "")
    }
    
    @IBAction func automateOn() {
        send(topic: "naos/set/automate", payload: "1")
    }
    
    @IBAction func automateOff() {
        send(topic: "naos/set/automate", payload: "0")
    }
    
    @IBAction func moveUp() {
        send(topic: "move", payload: "up")
    }
    
    @IBAction func moveDown() {
        send(topic: "move", payload: "down")
    }
    
    @IBAction func calibrate() {
        send(topic: "calibrate", payload: "")
    }
    
    @IBAction func flash() {
        send(topic: "flash", payload: "512 512 512 512 500")
    }
    
    @IBAction func disco() {
        let red = String(Int(arc4random_uniform(1023)))
        let green = String(Int(arc4random_uniform(1023)))
        let blue = String(Int(arc4random_uniform(1023)))
        
        send(topic: "fade", payload: String(format: "%@ %@ %@ 0 500", red, green, blue))
    }
    
    @IBAction func back(_ sender: Any) {
        dismiss(animated: true, completion: nil)
    }
    
    @IBAction func move(sender: UIPanGestureRecognizer) {
        // get change and calculate new position
        var change = sender.translation(in: view).y
        var newPosition = CGFloat(position) - change
        
        // perform bounds check
        if newPosition < 25 {
            newPosition = 25
            change = CGFloat(position) - newPosition
        } else if newPosition > 250 {
            newPosition = 250
            change = CGFloat(position) - newPosition
        }
        
        if sender.state == .began {
            // show view
            moveView!.alpha = 1
        }
        
        if sender.state == .changed || sender.state == .began {
            // set frame
            moveView!.frame = CGRect(x: view.frame.width/2 + 100, y: lightView!.frame.origin.y + lightView!.frame.height + change, width: 100, height: 1)
            
            // set label
            moveLabel?.text = String(format: "%.1f", newPosition)
        }
        
        if sender.state == .ended {
            // hide view
            moveView!.alpha = 0
            
            // send move command
            send(topic: "move", payload: String(format: "%.1f", newPosition))
        }
    }
    
    // Helpers
    
    func send(topic: String, payload: String) {
        // send message using the main view controller
        if let mvc = mainVC {
            mvc.send(id: id, topic: topic, payload: payload)
        }
    }
    
    // UIViewController
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
}
