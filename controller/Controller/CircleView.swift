//
//  Circle.swift
//  Controller
//
//  Created by Joël Gähwiler on 08.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit

class CircleView: UIView {
    var arc: UIView?
    var text: UILabel?
    
    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
        prepare()
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        prepare()
    }
    
    func prepare() {
        // set background color
        backgroundColor = UIColor.clear
    
        // add arc view
        arc = UIView(frame: CGRect(origin: CGPoint.zero, size: frame.size))
        arc!.layer.cornerRadius = frame.width/2
        arc!.layer.borderWidth = 1.0
        arc!.layer.borderColor = UIColor.white.cgColor
        addSubview(arc!)
        
        // add text view
        text = UILabel(frame: CGRect(origin: CGPoint.zero, size: frame.size))
        text!.textAlignment = .center
        text!.textColor = .white
        text!.text = "0.0"
        addSubview(text!)
    }
}
