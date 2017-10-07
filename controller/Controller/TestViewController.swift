//
//  TestViewController.swift
//  Controller
//
//  Created by Joël Gähwiler on 08.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit

class TestViewController: UIViewController {
    var circleView: CircleView?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        circleView = CircleView(frame: CGRect(x: (view.frame.width/4)-25, y: (view.frame.height/2)-25, width: 50, height: 50))
        view.addSubview(circleView!)
    }
    
    @IBAction func back(_ sender: Any) {
        dismiss(animated: true, completion: nil)
    }
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
}
