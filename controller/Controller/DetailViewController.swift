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
    var ropeView: UIView?
    var lightView: UIView?
    var floorView: UIView?
    var objectView: UIView?
    
    var position: Double = 250
    var distance: Double = 65
    var motion: Bool = false
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
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
    
    @IBAction func back(_ sender: Any) {
        dismiss(animated: true, completion: nil)
    }
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
}
